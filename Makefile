
# create a 10MB image
# format the image as FAT32
create_test_img: 
	dd if=/dev/zero of=test.img bs=1M count=60 
	mkfs.vfat -F 32 test.img 

mount_test_img: 
	sudo mkdir -p /mnt/test
	sudo mount -o loop test.img /mnt/test

umount_test_img: 
	sudo umount /mnt/test

restore: 
	g++ main.cpp -o restore.out 
	./restore ./test.img

print_disk:
	g++ printdisk.cpp -o printdisk.out
	./printdisk ./test.img
