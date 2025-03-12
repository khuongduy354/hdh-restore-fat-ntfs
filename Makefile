
TEST_IMAGE := ./test.img
MOUNT_POINT := /mnt/test
# create a 10MB image
# format the image as FAT32
run_full_flow:
	dd if=/dev/zero of=test.img bs=1M count=60
	mkfs.vfat -F 32 test.img 
	sudo mkdir -p /mnt/test

	@if ! mount | grep -q "$(TEST_IMAGE) $(MOUNT_POINT)"; then \
		sudo mount -o loop "$(TEST_IMAGE)" "$(MOUNT_POINT)" || true; \
	else \
		echo "Image already mounted, continuing..."; \
	fi


	# 
	# 
	# create edited text file
	echo "hi, i'm duy" | sudo tee /mnt/test/test.txt > /dev/null 
	#
	# showing the file in the mounted image
	sudo cat /mnt/test/test.txt
	#
	#
	# now remove it 
	sudo rm /mnt/test/test.txt 
	#
	# it should disappear
	sudo ls /mnt/test	 
	#
	#
	# try restore
	./restore.out ./test.img > output.txt
	#
	#
	# it should be back.
	sudo ls /mnt/test	 
	# 
	# 
	# 
	sudo umount /mnt/test




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
