function ret = smu_flash_erase(serial)
struct = create_cmd_struct(10,hex2dec('affe'),hex2dec('dead'),uint32(hex2dec('badeaffe')));
send_cmd_struct(serial,struct);
pause(5);
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    ret = smu_flash_erase(serial);
    return
end
end