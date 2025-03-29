
TEST_IMAGE := ./test.img
MOUNT_POINT := /mnt/test
# create a 10MB image
# format the image as FAT32
setup_test_image_with_deleted_file:
	dd if=/dev/zero of=test.img bs=1M count=10
	mkfs.vfat -F 32 test.img 
	sudo mkdir -p /mnt/test

	@if ! mount | grep -q "$(TEST_IMAGE) $(MOUNT_POINT)"; then \
		sudo mount -o loop "$(TEST_IMAGE)" "$(MOUNT_POINT)" || true; \
	else \
		echo "Image already mounted, continuing..."; \
	fi


	sudo vi -n /mnt/test/test.txt
	# sudo rm /mnt/test/test.txt 

	ls /mnt/test/



restore: 
	# try restore
	./restore.out ./test.img > output.txt
	#
	# 

	# remount 
	sudo umount /mnt/test 
	sudo mount -o loop test.img /mnt/test
	# it should be back.
	sudo ls /mnt/test	 


print_hex:
	xxd -l 3000000 ./test.img > hex.txt 


create_test_img: 
	dd if=/dev/zero of=test.img bs=1M count=60
	mkfs.vfat -F 32 test.img 

mount_test_img: 
	sudo mkdir -p /mnt/test
	sudo mount -o loop test.img /mnt/test

umount_test_img: 
	sudo umount /mnt/test

compile_restore: 
	g++ main.cpp -o restore.out 

print_disk:
	g++ printdisk.cpp -o printdisk.out
	./printdisk.out ./test.img
