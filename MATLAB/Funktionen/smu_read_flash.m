function ret = smu_read_flash(serial,address,size)

struct = create_cmd_struct(4,size,0,uint32(address));
send_cmd_struct(serial,struct);
pause(0.1);
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    smu_read_flash(serial,address,size);
    return
end
ret = struct(4:end-7);
end