#include"header.h"
// Cấu trúc lưu thông tin tệp

// void printMFTRawData(const uint8_t* data, int size_T) {
//     for (size_t i = 0; i < size_T; ++i) {
//         // In dữ liệu dưới dạng raw (byte theo hex)
//         cout << hex << uppercase << setw(2) << setfill('0') << (int)data[i] << " ";

//         // Mỗi dòng in ra 16 byte
//         if ((i + 1) % 16 == 0) {
//             cout << endl;
//         }
//     }
//     cout << endl;
// }

void printMFTRawData( vector<uint8_t> data) {
    for (size_t i = 0; i < data.size(); ++i) {
        // In dữ liệu dưới dạng raw (byte theo hex)
        cout << hex << uppercase << setw(2) << setfill('0') << (int)data[i] << " ";

        // Mỗi dòng in ra 16 byte
        if ((i + 1) % 16 == 0) {
            cout << endl;
        }
    }
    cout << endl;
}


string wstringToString(const wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    string str(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, NULL, NULL);
    return str;
}

uint32_t swapEndian(uint32_t val) {
    return ((val >> 24) & 0xFF) |   // Byte 1 -> Byte 4
           ((val >> 8) & 0xFF00) |  // Byte 2 -> Byte 3
           ((val << 8) & 0xFF0000) | // Byte 3 -> Byte 2
           ((val << 24) & 0xFF000000); // Byte 4 -> Byte 1
}

// Đọc dữ liệu từ ổ đĩa

bool readDiskData(const char* disk_path, std::vector<uint8_t>& buffer, uint64_t byteOffset, DWORD byteToRead) {
     // Adjust if your system uses 512
    uint64_t alignedOffset = (byteOffset / SECTOR_SIZE) * SECTOR_SIZE;
    uint64_t offsetDiff = byteOffset - alignedOffset;
    uint64_t alignedReadSize = ((byteToRead + offsetDiff + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;

    HANDLE hDrive = CreateFileA(
        disk_path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL
    );

    if (hDrive == INVALID_HANDLE_VALUE) {
        std::cout << "Error opening disk: " << GetLastError() << std::endl;
        return false;
    }

    LARGE_INTEGER li;
    li.QuadPart = alignedOffset;
    if (!SetFilePointerEx(hDrive, li, NULL, FILE_BEGIN)) {
        cout << "Error setting file pointer: " << GetLastError() << std::endl;
        CloseHandle(hDrive);
        return false;
    }

    vector<uint8_t> tempBuffer(alignedReadSize);
    DWORD bytesRead = 0;

    if (!ReadFile(hDrive, tempBuffer.data(), alignedReadSize, &bytesRead, NULL)) {
        cout << "ReadFile failed: " << GetLastError() << std::endl;
        CloseHandle(hDrive);
        return false;
    }
    CloseHandle(hDrive);

    if (bytesRead < offsetDiff + byteToRead) {
        cout << "Read fewer bytes than expected." << std::endl;
        return false;
    }

    // Trich xuat duy nhat tu vi tri tren
    buffer.assign(
        tempBuffer.begin() + offsetDiff,
        tempBuffer.begin() + offsetDiff + byteToRead
    );
    
    return true;
}





uint64_t getMFTOffset(const vector<uint8_t>& buffer){
    uint64_t mftCluster = 0;
    WORD bytesPerSector = 0;
    uint8_t sectorPerCluster = 0;

    memcpy(&mftCluster, &buffer[0x30], 8);
    memcpy(&bytesPerSector, &buffer[0x0B], 2);
    memcpy(&sectorPerCluster, &buffer[0x0D], 1);

    return mftCluster * bytesPerSector * sectorPerCluster;
}

// Trích xuất thông tin file từ MFT Record
string ConvertUTF16toUTF8(const char16_t* utf16Str, int length) {
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> converter;
    return converter.to_bytes(u16string(utf16Str, length));
}

// Hàm kiểm tra file hợp lệ (Không ẩn, không phải hệ thống)
bool IsValidFile(uint32_t flags) {
    return !(flags & 0x02) && !(flags & 0x04); // Không phải HIDDEN hoặc SYSTEM
}


vector<DataRun> ParseDataRuns(uint8_t* dataRunPtr) {
    vector<DataRun> runs;
    int offset = 0;
    DataRun run;
    int64_t currentLCN = 0;

    while (dataRunPtr[offset] != 0x00) {
        uint8_t header = dataRunPtr[offset];
        uint8_t len_size = header & 0x0F;
        uint8_t offset_size = (header >> 4) & 0x0F;
        offset++;

        uint64_t run_length = 0;
        for (int i = 0; i < len_size; i++) {
            run_length |= (uint64_t)dataRunPtr[offset + i] << (i * 8);
        }

        int64_t run_offset = 0;
        uint64_t tmp = 0;
        for (int i = 0; i < offset_size; i++) {
            tmp |= (uint64_t)dataRunPtr[offset + len_size + i] << (i * 8);
        }

        // Nếu offset âm (bit cuối là 1), cần sign-extend
        if (offset_size > 0 && (dataRunPtr[offset + len_size + offset_size - 1] & 0x80)) {
            run_offset = (int64_t)(tmp | (~((1LL << (offset_size * 8)) - 1)));
        } else {
            run_offset = tmp;
        }

        currentLCN += run_offset;
        run.clusterLength = run_length;
        run.clusterOffset = currentLCN;
        runs.push_back(run);
        offset += len_size + offset_size;
    }

    return runs;
}

vector<uint8_t> ReadNonResidentData(const char* diskPath, const vector<DataRun>& dataRuns, uint64_t fileSize) {
    vector<uint8_t> fileData;
    uint64_t bytesRead = 0;
    uint64_t clusterSize = 4096;  // Assuming 4KB clusters

    for (const auto& run : dataRuns) {
        uint64_t bytesToRead = run.clusterLength * clusterSize;
        if (bytesRead + bytesToRead > fileSize) {
            bytesToRead = fileSize - bytesRead;  // Ensure we don't read past the file size
        }
        
        vector<uint8_t> buffer(bytesToRead);
        uint64_t offset = run.clusterOffset * clusterSize;
        if (!readDiskData(diskPath, buffer, offset, bytesToRead)) {
            return {};  // Return empty vector on error
        }

        fileData.insert(fileData.end(), buffer.begin(), buffer.end());
        bytesRead += bytesToRead;

        // If we have read the entire file, stop early
        if (bytesRead >= fileSize) break;
    }

    return fileData;
}



void ParseAttributes(uint8_t* mftRecord, size_t recordSize, vector<FileInfo> &file_list, const char* disk_path) {
    NTFS_MFT_RECORD_HEADER mftHeader;
    memcpy(&mftHeader, mftRecord, sizeof(NTFS_MFT_RECORD_HEADER));
    // Kiểm tra chữ ký MFT hợp lệ
    uint16_t firstAttrOffset = mftHeader.FirstAttrOffset;
    uint32_t signature = swapEndian(mftHeader.Signature);
    uint16_t flags = mftHeader.Flags;

    if (mftRecord == nullptr) {
        cerr << "Error: Null MFT record pointer!" << endl;
        return;
    }

    if (recordSize < sizeof(NTFS_MFT_RECORD_HEADER)) {
        cerr << "Error: MFT record too small!" << endl;
        return;
    }

    if (signature != 0x46494C45) {
        return;
    }

    if ((flags & 0x01)) return;  // Skip file ko bị xóa
    uint8_t* attrPtr = mftRecord + mftHeader.FirstAttrOffset;
    FileInfo fileInfo;
    bool hasFileName = false;
    bool hasData = false;
    int count = 0;
    while (attrPtr < mftRecord + recordSize) {
        if(hasFileName == true && hasData == true)
            break;
        NTFS_ATTRIBUTE_HEADER* attr = (NTFS_ATTRIBUTE_HEADER*)attrPtr;
        uint8_t attr_isresident = attr->NonResident;
        if (attr->Length == 0) break; // tránh infinite loop
        if (attr->Type == 0xFFFFFFFF) break; // End marker

        // Xử lý $FILE_NAME (Type 0x30)
        if (attr->Type == 0x30 && attr_isresident == 0) {
            
            uint16_t contentOffset = ((NTFS_RESIDENT_ATTRIBUTE*)attr)->ContentOffset;
            NTFS_FILE_NAME_ATTRIBUTE* fileNameAttr = (NTFS_FILE_NAME_ATTRIBUTE*)(attrPtr + contentOffset);
            if (!IsValidFile(fileNameAttr->Flags)) return;  // Bỏ qua file ẩn & hệ thống
            WCHAR* namePtr = fileNameAttr->FileName;
            size_t validNameLength = fileNameAttr->NameLength;
            fileInfo.FileName = wstring(namePtr, validNameLength);
            fileInfo.fileSize = fileNameAttr->RealSize;
            hasFileName = true;
            
        }
        // Kiểm tra nếu file có dữ liệu (Resident / Non-Resident)
        if (attr->Type == 0x80) {
            if((attr->NonResident) == 0){
                fileInfo.IsResident = 1;
                NTFS_RESIDENT_ATTRIBUTE* resAttr = (NTFS_RESIDENT_ATTRIBUTE*)attr;
                uint32_t dataSize = resAttr->ContentSize;
                uint16_t dataOffset = resAttr->ContentOffset;
                // Copy file content
                if (dataSize > 0 && (attrPtr + dataOffset + dataSize) <= (mftRecord + recordSize)) {
                    fileInfo.Data.assign(attrPtr + dataOffset, attrPtr + dataOffset + dataSize);
                }
            }
            else{
                fileInfo.IsResident = 0;
                NTFS_NONRESIDENT_ATTRIBUTE* nonResAttr = (NTFS_NONRESIDENT_ATTRIBUTE*)attr;
                uint64_t dataRunOffset = (nonResAttr->DataRunOffset);
                // cout << dataRunOffset << endl;
                uint64_t fileSize = (nonResAttr->ActualSize);
                uint8_t* dataRunPtr = attrPtr + dataRunOffset; // Pointer to Data Runs
                // Decode Data Runs to get LCNs (Logical Cluster Numbers)
                vector<DataRun> dataRuns = ParseDataRuns(dataRunPtr);
                
                // Read file content using cluster info
                vector<uint8_t> fileData = ReadNonResidentData(disk_path, dataRuns, fileSize);
                fileInfo.fileSize = fileSize;
                fileInfo.Data = fileData;
            }
            hasData = true;
        }
        
        attrPtr += attr->Length;
        count++;
        
    }
    if (hasFileName && hasData) {
        file_list.push_back(fileInfo);
    }
    
    
}

// Hàm đọc toàn bộ MFT
vector<FileInfo> listMFTRecords(const char* disk_path, const vector<uint8_t>& buffer, int maxEntries ){
    vector<FileInfo> fileList;
    uint64_t mft_0_Offset = getMFTOffset(buffer);
    uint64_t currentOffset = mft_0_Offset;

    for (int i = 0; i < maxEntries; i++) {
        vector<uint8_t> mftEntry;
        if (readDiskData(disk_path, mftEntry, currentOffset, 1024) ) { // MFT Entry size = 1024 bytes
            ParseAttributes(mftEntry.data(), 1024, fileList, disk_path);
        } else {
            cout << "Error reading MFT entry " << i << endl;
            break;
        }
        currentOffset += 1024; // Tăng offset mỗi lần đọc
    }
    return fileList;
}

bool RecoverFile(const FileInfo& fileInfo, const wstring& outputDir) {
    if (fileInfo.FileName.empty()) {
        cerr << "Error: No file name found!" << endl;
        return false;
    }

    // Create output file path
    wstring outputPath = outputDir + L"\\" +  fileInfo.FileName;
    
    ofstream outFile(wstringToString(outputPath), ios::binary);
    
    if (!outFile.good()) {
        wcout << "Error: Cannot create output file: " << outputPath << endl;
        return false;
    }

   // Write file data
    if (fileInfo.IsResident) {
        outFile.write(reinterpret_cast<const char*>(fileInfo.Data.data()), fileInfo.Data.size());
    } 
    else {
        for (const auto& cluster : fileInfo.Data) {
            outFile.write(reinterpret_cast<const char*>(&cluster), 1); // Write non-resident data
        }
    }

    outFile.close();
    return true;
}
