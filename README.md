# Khôi phục dữ liệu NTFS, FAT   

# How to run  

- On linux environment

1. `make create_test_img` 
    - Tạo ổ đĩa ảo format FAT32

2. `make mount_test_img `

    - Ổ đĩa ảo được mount ở /mnt/test, thử tạo file ở đây bằng lệnh touch hay mkdir  

3. `restore.out ./test.img`

    - Sẽ liệt kê tất cả file bị xóa và đã khôi phục


4. `make umount_test_img`
    - Chạy lệnh này để unmount đĩa, sau khi chạy xong chương trình

- Helper: 
    - printdisk, chạy file này với input là disk-image để in ra toàn bộ volume 
VD: `printdisk.out ./test.img`
    - xxd -l 512 ./test.img  :cli linux có sẵn, hoạt động tương tự trên



