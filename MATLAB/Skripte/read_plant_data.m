stm32 = connect_smu;

smu_read_free_cap(stm32)
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
data_width = 22;
time = zeros(1,((pages*2046)/data_width));
power = zeros(1,((pages*2046)/data_width));
j=1;
for i=1:data_width:((pages*2046))
    power(1,j) = -fourbytestodec_signed(flip(smu_data(1,(i+9):(i+12))));
    time(1,j) = fourbytestodec_signed(flip(smu_data(1,(i+13):(i+16))));
    j=j+1;
end
plot(time(1,1:end),power(1,1:end))

