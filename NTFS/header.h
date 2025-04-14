#pragma once
// Đảm bảo alignment chính xác
#include <windows.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdint>
#include <string>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <fstream>
#include <cstring>

using namespace std;

#pragma pack(push, 1) 
#define SECTOR_SIZE 512
#define MFT_ENTRY_SIZE 1024
#define MFT_FLAGS_OFFSET 0x16  // Offset của Flags trong MFT Entry
#define FILE_NAME_OFFSET 0x38   // Vị trí gần đúng của File Name trong MFT
#define FILE_NAME_LENGTH_OFFSET 0x40 // Offset độ dài tên file

struct NTFS_MFT_RECORD_HEADER {
    uint32_t Signature;     // "FILE" (0x46494C45)
    uint16_t UpdateOffset;  // Update sequence offset
    uint16_t UpdateSize;    // Update sequence size
    uint64_t LogSequence;   // Log sequence number
    uint16_t SequenceNum;   // Sequence number
    uint16_t HardLinkCount; // Hard link count
    uint16_t FirstAttrOffset; // First attribute offset
    uint16_t Flags;         // 0x01 = In Use, 0x02 = Directory
    uint32_t UsedSize;      // Used entry size
    uint32_t AllocatedSize; // Allocated entry size
    uint64_t FileRef;       // Base MFT record reference
    uint16_t NextAttrID;    // Next attribute ID
    uint16_t Padding;       // Align to 4 bytes
    uint32_t MFTRecordNumber; // MFT record number
};

// NTFS Attribute Header (Generic)
struct NTFS_ATTRIBUTE_HEADER {
    uint32_t Type;         // Attribute type (e.g., $FILE_NAME = 0x30)
    uint32_t Length;       // Length of the attribute (including header)
    uint8_t NonResident;   // 0 = Resident, 1 = Non-Resident
    uint8_t NameLength;    // Name length (if named attribute)
    uint16_t NameOffset;   // Offset to name (if any)
    uint16_t Flags;        // Flags (compressed, encrypted, etc.)
    uint16_t AttributeId;     // Attribute ID within the file record
};

struct NTFS_DATA_ATTRIBUTE{
    uint32_t data_attribute_type;
    uint32_t length;
    uint8_t non_resident;
};

// Resident Attribute Header (if NonResident = 0)
struct NTFS_RESIDENT_ATTRIBUTE {
    NTFS_ATTRIBUTE_HEADER Header;
    uint32_t ContentSize;  // Size of attribute content
    uint16_t ContentOffset;// Offset to content
    uint8_t IndexedFlag;   // 0 = Not indexed, 1 = Indexed
    uint8_t Padding;       // Align to 4 bytes
};

// Non-Resident Attribute Header (if NonResident = 1)
struct NTFS_NONRESIDENT_ATTRIBUTE {
    NTFS_ATTRIBUTE_HEADER Header;
    uint64_t StartVCN;     // Virtual Cluster Number (start)
    uint64_t EndVCN;       // Virtual Cluster Number (end)
    uint16_t DataRunOffset;// Offset to Data Runs
    uint16_t CompressionUnitSize;
    uint32_t Padding;
    uint64_t AllocatedSize;
    uint64_t ActualSize;
    uint64_t InitializedSize;
};

// $FILE_NAME Attribute (Type = 0x30)
struct NTFS_FILE_NAME_HEADER {
    uint32_t type;
    uint32_t length;
    uint8_t isresident;
    uint8_t name_length;
    uint16_t name_pos;
    uint16_t flags;
    uint16_t identity;
    uint32_t size_of_data;
    uint16_t data_start_offset;
};

struct NTFS_FILE_NAME_ATTRIBUTE {
    uint64_t ParentRef;    // Parent directory reference
    uint64_t CreatedTime;  // File creation timestamp
    uint64_t ModifiedTime; // Last modified timestamp
    uint64_t MFTChangedTime; // MFT record change timestamp
    uint64_t LastAccessTime; // Last access timestamp
    uint64_t AllocatedSize;// Allocated size of file
    uint64_t RealSize;     // Actual file size
    uint32_t Flags;        // File flags (readonly, hidden, etc.)
    uint32_t ReparseTag;   // Reparse point tag
    uint8_t NameLength;    // File name length (in characters)
    uint8_t NameType;      // 0 = POSIX, 1 = Win32, 2 = DOS, 3 = both
    WCHAR FileName[1];// File name (UTF-16)
};

struct DataRun {
    uint64_t clusterOffset; // Relative offset from previous LCN
    uint64_t clusterLength; // Number of clusters
};


// Structure to hold file information
struct FileInfo {
    wstring FileName;  // File name in UTF-16
    LONGLONG fileSize;
    uint64_t CreatedTime;   // File creation timestamp
    uint64_t ModifiedTime;  // Last modified timestamp
    bool IsResident;
    bool IsDeleted;
    vector<uint8_t> Data; // File data content
};

// struct StandardInfomation{
//     uint32 
// };
#pragma pack(pop)  // Khôi phục alignment mặc định

void printMFTRawData( vector<uint8_t> data);
string wstringToString(const wstring& wstr);
uint32_t swapEndian(uint32_t val);
bool readDiskData(const char* disk_path, vector<uint8_t>& buffer, uint64_t byteOffset, DWORD byteToRead);
uint64_t getMFTOffset(const vector<uint8_t>& buffer);
string ConvertUTF16toUTF8(const char16_t* utf16Str, int length);
bool IsValidFile(uint32_t flags);
vector<DataRun> ParseDataRuns(uint8_t* dataRunPtr);
vector<uint8_t> ReadNonResidentData(const char* diskPath, const vector<DataRun>& dataRuns, uint64_t fileSize);
void ParseAttributes(uint8_t* mftRecord, size_t recordSize, vector<FileInfo> &file_list, const char* disk_path);
vector<FileInfo> listMFTRecords(const char* disk_path, const vector<uint8_t>& buffer, int maxEntries = 50);
bool RecoverFile(const FileInfo& fileInfo, const wstring& outputDir);