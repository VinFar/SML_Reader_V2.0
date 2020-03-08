
stm32 = connect_smu;
data_width = 18;
smu_read_free_cap(stm32)

addresses = smu_read_flash_address_plant(stm32);
start_address = addresses(3);
end_address = addresses(1);
pages = (end_address-start_address)/2048
pages = uint32(pages);
size = uint32(floor(2048/data_width))*data_width

smu_data_plant = zeros(1,pages*size);
a=1;
tic
for i=1:1:pages
   smu_data_plant(1,a:(a+size-1)) = smu_read_flash(stm32,start_address,size);
   start_address = start_address + 2048;
   a=a+size
end
toc

time_plant = zeros(1,((pages*size)/data_width));
power_plant = zeros(1,((pages*size)/data_width));
j=1;
for i=1:data_width:((pages*size))
    power_plant(1,j) = -fourbytestodec_signed(flip(smu_data_plant(1,(i+9):(i+12))));
    time_plant(1,j) = fourbytestodec_signed(flip(smu_data_plant(1,(i+13):(i+16))));
    j=j+1;
end
time_plant = time_plant - time_plant(1,1)

plot(time_plant(1,1:end),power_plant(1,1:end))
hold on

addresses = smu_read_flash_address_main(stm32);
start_address = addresses(3);
end_address = addresses(1);
pages = (end_address-start_address)/2048
pages = uint32(pages);

smu_data_main = zeros(1,pages*size);
a=1;
tic
for i=1:1:pages
   smu_data_main(1,a:(a+size-1)) = smu_read_flash(stm32,start_address,size);
   start_address = start_address + 2048;
   a=a+size
end
toc

time_main = zeros(1,((pages*size)/data_width));
power_main = zeros(1,((pages*size)/data_width));
j=1;
for i=1:data_width:((pages*size))
    power_main(1,j) = -fourbytestodec_signed(flip(smu_data_main(1,(i+9):(i+12))));
    time_main(1,j) = fourbytestodec_signed(flip(smu_data_main(1,(i+13):(i+16))));
    j=j+1;
end
time_main = time_main - time_main(1,1)
plot(time_main(1,1:end),power_main(1,1:end))
grid on
grid minor

