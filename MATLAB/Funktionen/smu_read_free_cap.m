function ret = smu_read_free_cap(serial)
data = smu_read_flash_address_plant(serial);
ret = (((data(2)-data(1))/data(3)))*100;
end