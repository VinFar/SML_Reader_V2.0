function ret = smu_get_unix_time(serial)

struct = create_cmd_struct(9,0,0,0);
send_cmd_struct(serial,struct);
pause(0.1);
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    ret = smu_get_unix_time(serial);
    return
end
ret = fourbytestodec(flip(struct(4:7)));
end