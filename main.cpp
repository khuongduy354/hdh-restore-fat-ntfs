#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace std;

#pragma pack(push, 1) // Ensure no padding

struct FAT32BootSector
{
  uint8_t jumpBoot[3];
  char oemName[8];
  uint16_t bytesPerSector;
  uint8_t sectorsPerCluster;
  uint16_t reservedSectors;
  uint8_t numFATs;
  uint16_t rootEntries;    // Obsolete for FAT32
  uint16_t totalSectors16; // Obsolete for FAT32
  uint8_t mediaDescriptor;
  uint16_t sectorsPerFAT16; // Obsolete for FAT32
  uint16_t sectorsPerTrack;
  uint16_t numHeads;
  uint32_t hiddenSectors;
  uint32_t totalSectors32;
  uint32_t sectorsPerFAT32;
  uint16_t flags;
  uint16_t fatVersion;
  uint32_t rootCluster;
  uint16_t fsInfoSector;
  uint16_t backupBootSector;
  uint8_t reserved[12];
  uint8_t driveNumber;
  uint8_t reserved1;
  uint8_t bootSignature;
  uint32_t volumeID;
  char volumeLabel[11];
  char fsType[8];
  uint8_t bootCode[420]; // Simplified, adjust as needed
  uint16_t bootSectorSignature;
};

struct FAT32DirectoryEntry
{
  char filename[8];
  char extension[3];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creationTimeTenth;
  uint16_t creationTime;
  uint16_t creationDate;
  uint16_t lastAccessDate;
  uint16_t firstClusterHigh;
  uint16_t lastWriteTime;
  uint16_t lastWriteDate;
  uint16_t firstClusterLow;
  uint32_t fileSize;
};

#pragma pack(pop)

bool isDeletedEntry(const FAT32DirectoryEntry &entry)
{
  return (uint8_t)entry.filename[0] == 0xE5;
}

std::string getFilename(const FAT32DirectoryEntry &entry)
{
  std::string filename(entry.filename, 8);
  filename.erase(std::find(filename.begin(), filename.end(),
                           ' ')); // Remove trailing spaces
  std::string extension(entry.extension, 3);
  extension.erase(std::find(extension.begin(), extension.end(),
                            ' ')); // Remove trailing spaces

  if (!extension.empty())
  {
    filename += "." + extension;
  }
  return filename;
}

void printEntry(FAT32DirectoryEntry entry)
{
  cout << "Filename: " << entry.filename << endl;
  cout << "Attributes: " << hex << (int)entry.attributes << dec << endl;
  cout << "File Size: " << entry.fileSize << " bytes" << endl;
  cout << "First Cluster Low: " << entry.firstClusterLow << endl;
  cout << "First Cluster High: " << entry.firstClusterHigh << endl;
}

void printBootSector(FAT32BootSector i)
{
  cout << "Jump Boot: " << i.jumpBoot << endl;
  cout << "OEM Name: " << i.oemName << endl;
  cout << "Bytes Per Sector: " << i.bytesPerSector << endl;
  cout << "Sectors Per Cluster: " << i.sectorsPerCluster << endl;
  cout << "Reserved Sectors: " << i.reservedSectors << endl;
  cout << "Number of FATs: " << i.numFATs << endl;
  cout << "Root Entries: " << i.rootEntries << endl;
  cout << "Total Sectors 16: " << i.totalSectors16 << endl;
  cout << "Media Descriptor: " << i.mediaDescriptor << endl;
  cout << "Sectors Per FAT 16: " << i.sectorsPerFAT16 << endl;
  cout << "Sectors Per Track: " << i.sectorsPerTrack << endl;
  cout << "Number of Heads: " << i.numHeads << endl;
  cout << "Hidden Sectors: " << i.hiddenSectors << endl;
  cout << "Total Sectors 32: " << i.totalSectors32 << endl;
  cout << "Sectors Per FAT 32: " << i.sectorsPerFAT32 << endl;
  cout << "Flags: " << i.flags << endl;
  cout << "FAT Version: " << i.fatVersion << endl;
  cout << "Root Cluster: " << i.rootCluster << endl;
  cout << "FS Info Sector: " << i.fsInfoSector << endl;
  cout << "Backup Boot Sector: " << i.backupBootSector << endl;
  cout << "Reserved: " << i.reserved << endl;
  cout << "Drive Number: " << i.driveNumber << endl;
  cout << "Reserved 1: " << i.reserved1 << endl;
  cout << "Boot Signature: " << i.bootSignature << endl;
  cout << "Volume ID: " << i.volumeID << endl;
  cout << "Volume Label: " << i.volumeLabel << endl;
  cout << "FS Type: " << i.fsType << endl;
  cout << "Boot Code: " << i.bootCode << endl;
  cout << "Boot Sector Signature: " << i.bootSectorSignature << endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <disk_image>" << std::endl;
    return 1;
  }

  std::ifstream disk(argv[1], std::ios::binary);
  if (!disk.is_open())
  {
    std::cerr << "Error opening disk image: " << argv[1] << std::endl;
    return 1;
  }

  FAT32BootSector bootSector;
  disk.read(reinterpret_cast<char *>(&bootSector), sizeof(bootSector));

  if (bootSector.bootSectorSignature != 0xAA55)
  {
    std::cerr << "Invalid boot sector signature." << std::endl;
    return 1;
  }

  uint32_t firstDataSector = bootSector.reservedSectors +
                             (bootSector.numFATs * bootSector.sectorsPerFAT32);
  uint32_t rootDirectorySector =
      firstDataSector +
      (bootSector.rootCluster - 2) * bootSector.sectorsPerCluster;
  uint32_t bytesPerCluster =
      bootSector.bytesPerSector * bootSector.sectorsPerCluster;

  printBootSector(bootSector);

  std::vector<FAT32DirectoryEntry> deletedEntries;
  std::vector<FAT32DirectoryEntry> entries;

  disk.seekg(rootDirectorySector * bootSector.bytesPerSector);

  FAT32DirectoryEntry entry;
  int entries_lim = 10;
  int entries_count = 0;

  while (disk.read(reinterpret_cast<char *>(&entry), sizeof(entry)))
  {
    if (entries_count == entries_lim)
      break;
    entries_count++;
    printEntry(entry);

    if (isDeletedEntry(entry))
    {
      deletedEntries.push_back(entry);
    }
    else if (entry.filename[0] != 0x00 &&
             entry.filename[0] !=
                 0x2E)
    { // 0x00 is end of directory, 0x2E is . or ..
      entries.push_back(entry);
    }
  }

  std::cout << "Deleted Files:" << std::endl;
  for (const auto &deletedEntry : deletedEntries)
  {
    std::cout << deletedEntry.filename << std::endl;
    std::cout << "  File Size: " << deletedEntry.fileSize << " bytes"
              << std::endl;
    std::cout << "  First Cluster Low: " << deletedEntry.firstClusterLow
              << std::endl;
    std::cout << "  First Cluster High: " << deletedEntry.firstClusterHigh
              << std::endl;

    // basic attempt at recovering. This is not a proper recovery and often will
    // not work.
    bool cond = deletedEntry.fileSize > 0 && deletedEntry.firstClusterLow > 1;
    if (cond)
    {
      cout << "  Attempting recovery..." << endl;
      uint32_t cluster = deletedEntry.firstClusterLow;
      uint32_t sector =
          firstDataSector + (cluster - 2) * bootSector.sectorsPerCluster;

      string filename = deletedEntry.filename;
      std::string recoveredFilename = "recovered_" + filename;
      std::ofstream recoveredFile(recoveredFilename, std::ios::binary);

      if (recoveredFile.is_open())
      {
        cout << "  Recovered file: " << recoveredFilename << endl;
        disk.seekg(sector * bootSector.bytesPerSector);
        std::vector<char> buffer(bytesPerCluster);
        uint32_t bytesRemaining = deletedEntry.fileSize;

        while (bytesRemaining > 0)
        {
          uint32_t bytesToRead =
              std::min((uint32_t)buffer.size(), bytesRemaining);
          disk.read(buffer.data(), bytesToRead);
          recoveredFile.write(buffer.data(), bytesToRead);
          bytesRemaining -= bytesToRead;
          cluster = 0; // In a real recovery, you would read the FAT to find the
                       // next cluster.
          if (cluster < 2)
          {
            break; // stop if no more clusters.
          }
          sector =
              firstDataSector + (cluster - 2) * bootSector.sectorsPerCluster;
          disk.seekg(sector * bootSector.bytesPerSector);
        }
        recoveredFile.close();
        std::cout << "  Attempted recovery to: " << recoveredFilename
                  << std::endl;
      }
      else
      {
        std::cerr << "  Failed to open recovered file." << std::endl;
      }
    }
  }
  disk.close();
  return 0;
}
