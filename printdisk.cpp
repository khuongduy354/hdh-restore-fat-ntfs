#include <cctype> // For isprint()
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

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

  const int bytesPerRow = 16;
  std::vector<uint8_t> buffer(bytesPerRow);
  int offset = 0;

  int max_lines = 5000;
  int lines = 0;

  std::cout << "Offset   :00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   | ASCII" << std::endl;
  while (disk.read(reinterpret_cast<char *>(buffer.data()), bytesPerRow))
  {
    lines += 1;
    std::cout << std::setw(8) << std::setfill('0') << std::hex << offset
              << ": ";

    // Hex output
    for (int i = 0; i < bytesPerRow; ++i)
    {
      std::cout << std::setw(2) << std::setfill('0')
                << static_cast<int>(buffer[i]) << " ";
      if (i == 7)
      {
        std::cout << " "; // Add extra space after 8 bytes
      }
    }

    std::cout << " | ";

    // ASCII output
    for (int i = 0; i < bytesPerRow; ++i)
    {
      if (std::isprint(buffer[i]))
      {
        std::cout << static_cast<char>(buffer[i]);
      }
      else
      {
        std::cout << "."; // Replace non-printable characters with '.'
      }
    }

    std::cout << std::endl;
    offset += bytesPerRow;
    if (lines >= max_lines)
    {
      break;
    }
  }

  // Handle remaining bytes (if any)
  if (disk.gcount() > 0)
  {
    std::cout << std::setw(8) << std::setfill('0') << std::hex << offset
              << ": ";

    // Hex output for remaining bytes
    for (int i = 0; i < disk.gcount(); ++i)
    {
      std::cout << std::setw(2) << std::setfill('0')
                << static_cast<int>(buffer[i]) << " ";
      if (i == 7)
        std::cout << " ";
    }
    for (int i = disk.gcount(); i < bytesPerRow; ++i)
    {
      std::cout << "   "; // add padding if there are less than 16 bytes
      if (i == 7)
        std::cout << " ";
    }

    std::cout << " | ";

    // ASCII output for remaining bytes
    for (int i = 0; i < disk.gcount(); ++i)
    {
      if (std::isprint(buffer[i]))
      {
        std::cout << static_cast<char>(buffer[i]);
      }
      else
      {
        std::cout << ".";
      }
    }
    std::cout << std::endl;
  }

  disk.close();
  return 0;
}
