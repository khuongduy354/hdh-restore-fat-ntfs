#include"header.h"

int main() {
    char driveInput[10];  // Đủ để chứa "C:" hoặc "D:" + null
    cout << "Please enter the drive letter (e.g., C:, D:, F:): ";
    cin >> driveInput;
    cin.ignore();
    const char* prefix = "\\\\.\\";  // Tiền tố cần thiết
    int totalLength = strlen(prefix) + strlen(driveInput) + 1;  // +1 cho null terminator

    char* combinedPath = new char[totalLength];
    strcpy(combinedPath, prefix);
    strcat(combinedPath, driveInput);

    const char* disk_path = combinedPath;  // Thay bằng ổ đĩa cần quét
     // Giả sử MFT bắt đầu từ đây (cần lấy từ Boot Sector)
    DWORD mftSize = 1024 * 50; // Đọc 50 bản ghi đầu tiên

    vector<uint8_t> buffer;
    
    if (!readDiskData(disk_path, buffer, 0, 512)) {
        return -1;
    }
    // DWORD mftOffset = getMFTOffset(buffer);
    vector<FileInfo> fileList = listMFTRecords(disk_path, buffer, 50);
    
    // Hiển thị danh sách file
    cout << "Number of files deleted files: " << fileList.size() << endl;
    int stt = 1;
    for (const auto& file : fileList) {
        wcout << "File Number: " << stt << " | Type " << (file.IsResident ? "Resident" : "Non-Resident") 
                            << " | file size " << file.Data.size() << " bytes" << endl;
        stt++;
    }
    cout << "-------------------------------------------------------------------------------" << endl;
    cout << "Recover file processing...\n";
    int stt2 = 1;
    wstring outputDir;
    wcout << L"Please enter the output directory (e.g., E:\\\\RecoverFile): ";
    getline(wcin, outputDir);  // Dùng getline để lấy cả dòng (tránh mất ký tự `\`)

    wcout << L"Output directory set to: " << outputDir << endl;
    for (const auto& file : fileList) {
        if(RecoverFile(file, outputDir)){
            wcout << "File Recovered Number: " << stt2 << " | Type " << (file.IsResident ? "Resident" : "Non-Resident") 
                            << " | file size " << file.Data.size() << " bytes" << endl;
            stt2++;
        }
    }
    delete[] combinedPath;
    return 0;
}
