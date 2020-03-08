function ret = smu_read_flash_address_plant(serial)

struct = create_cmd_struct(3,0,0,0);
send_cmd_struct(serial,struct);
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    ret = smu_read_flash_address_main(serial);
    return
end
ret(1) = fourbytestodec(flip(struct(4:7)));
ret(2) = fourbytestodec(flip(struct(8:11)));
ret(3) = fourbytestodec(flip(struct(12:15)));
ret = double(ret);
end