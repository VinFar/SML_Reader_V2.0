stm32 = connect_smu;

smu_read_free_cap(stm32)
addresses = smu_read_flash_address_main(stm32);
start_address = addresses(3);
end_address = addresses(1);
pages = (end_address-start_address)/2048
pages = uint32(pages);

smu_data = zeros(1,pages*2048);
a=1;
tic
for i=1:1:pages
   smu_data(1,a:(a+2045)) = smu_read_flash(stm32,start_address,2046);
   start_address = start_address + 2048;
   a=a+2046
end
toc
data_width = 18;
time_plant = zeros(1,((pages*2046)/data_width));
power_main = zeros(1,((pages*2046)/data_width));
j=1;
for i=1:data_width:((pages*2046))
    power_main(1,j) = -fourbytestodec_signed(flip(smu_data(1,(i+9):(i+12))));
    time_plant(1,j) = fourbytestodec_signed(flip(smu_data(1,(i+13):(i+16))));
    j=j+1;
end
time_plant = time_plant - time_plant(1,1)
plot(time_plant(1,1:end),power_main(1,1:end))
hold on


addresses = smu_read_flash_address_plant(stm32);
start_address = addresses(3);
end_address = addresses(1);
pages = (end_address-start_address)/2048
pages = uint32(pages);

smu_data = zeros(1,pages*2048);
a=1;
tic
for i=1:1:pages
   smu_data(1,a:(a+2045)) = smu_read_flash(stm32,start_address,2046);
   start_address = start_address + 2048;
   a=a+2046
end
toc
data_width = 18;
time_main = zeros(1,((pages*2046)/data_width));
power_main = zeros(1,((pages*2046)/data_width));
j=1;
for i=1:data_width:((pages*2046))
    power_main(1,j) = -fourbytestodec_signed(flip(smu_data(1,(i+9):(i+12))));
    time_main(1,j) = fourbytestodec_signed(flip(smu_data(1,(i+13):(i+16))));
    j=j+1;
end
time_main = time_main - time_main(1,1)
plot(time_main(1,1:end),power_main(1,1:end))
grid on
grid minor

