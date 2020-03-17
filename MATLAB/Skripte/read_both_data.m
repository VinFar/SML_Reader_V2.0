tic
stm32 = connect_smu;
data_width = 18;
smu_read_free_cap(stm32)

addresses = smu_read_flash_address_plant(stm32);
start_address = addresses(3);
end_address = addresses(1);
pages = (end_address-start_address)/2048
pages = uint32(pages);
size = uint32(floor(2048/data_width))*data_width
if(pages == 0)
    display('no pages to read. Flash IC is empty!');
    return;
end

smu_data_plant = zeros(1,pages*size);
a=1;

for i=1:1:pages
    smu_data_plant(1,a:(a+size-1)) = smu_read_flash(stm32,start_address,size);
    start_address = start_address + 2048;
    a=a+size
end


time_plant = zeros(1,((pages*size)/data_width));
power_plant = zeros(1,((pages*size)/data_width));
meter_plant = zeros(1,((pages*size)/data_width));
j=1;
for i=1:data_width:(((pages-1)*size))
    power_plant(1,j) = -fourbytestodec_signed(flip(smu_data_plant(1,(i+9):(i+12))));
    time_plant(1,j) = fourbytestodec(flip(smu_data_plant(1,(i+13):(i+16))));
    meter_plant(1,j) = fourbytestodec(flip(smu_data_plant(1,(i+17):(i+20))));
    j=j+1;
end

time_plant = datetime(time_plant, 'convertfrom','posixtime');
% time_plant = time_plant - time_plant(1,1);

subplot(3,1,1)
stairs(time_plant(1,1:end-200),power_plant(1,1:end-200))
yyaxis right
plot(time_plant(1,1:end-200),meter_plant(1,1:end-200));
xlim([time_plant(1,1), time_plant(1,end-200)])
grid on
grid minor
title('Power of PV Plant');
xlabel('Time');
ylabel('Power in W');
xtickformat('dd.MM.yyyy HH:mm:ss')

addresses = smu_read_flash_address_main(stm32);
start_address = addresses(3);
end_address = addresses(1);
% pages = (end_address-start_address)/2048
% pages = uint32(pages);

smu_data_main = zeros(1,pages*size);
a=1;

for i=1:1:pages
    smu_data_main(1,a:(a+size-1)) = smu_read_flash(stm32,start_address,size);
    start_address = start_address + 2048;
    a=a+size
end


time_main = zeros(1,((pages*size)/data_width));
power_main = zeros(1,((pages*size)/data_width));
meter_p_main = zeros(1,((pages*size)/data_width));
meter_d_main = zeros(1,((pages*size)/data_width));

j=1;
for i=1:data_width:(((pages-1)*size))
    power_main(1,j) = -fourbytestodec_signed(flip(smu_data_main(1,(i+9):(i+12))));
    time_main(1,j) = fourbytestodec(flip(smu_data_main(1,(i+13):(i+16))));
    meter_p_main(1,j) = fourbytestodec(flip(smu_data_main(1,(i+17):(i+20))));
    meter_d_main(1,j) = fourbytestodec(flip(smu_data_main(1,(i+21):(i+24))));
    j=j+1;
end
time_main = datetime(time_main, 'convertfrom','posixtime');
% time_main = time_main - time_main(1,1)-692;

toc
githsubplot(3,1,2)

stairs(time_main(1,1:end-200),power_main(1,1:end-200))
ylabel('Power in W');
xlim([time_main(1,1), time_main(1,end-200)])
yyaxis right
% stairs(time_main(1,1:end-200),meter_p_main(1,1:end-200))
hold on
stairs(time_main(1,1:end-200),meter_d_main(1,1:end-200))
ylabel('Energy in kWh');
grid on
grid minor
title('Power of Main Smart Meter');
xlabel('Time');
xtickformat('dd.MM.yyyy HH:mm:ss')

power_diff = -(power_main - power_plant);

subplot(3,1,3)
stairs(time_main(1,1:end-200),power_diff(1,1:end-200))
xlim([time_main(1,1), time_main(1,end-200)])

grid on
grid minor
title('Power of House');
xlabel('Time');
ylabel('Power in W');
xtickformat('dd.MM.yyyy HH:mm:ss')
total_diff_in_sec = datetime(smu_get_unix_time(stm32), 'convertfrom','posixtime') - datetime('now')
