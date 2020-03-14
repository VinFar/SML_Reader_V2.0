function ret = smu_set_RTC(serial)
[TR,DR] = smu_get_RTC_registers();
struct = create_cmd_struct(8,0,0,uint32([TR,DR]));
send_cmd_struct(serial,struct);
pause(0.1);
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    ret = smu_set_RTC(serial);
    return
end
ret = struct(4:end-7);
end