function ret = smu_read_main_power(serial)
%reads the temperature of the incubation chamber
%upper command: 1
%lower command: 0

struct = create_cmd_struct(6,0,0,0);
send_cmd_struct(serial,struct);
serial.BaudRate=9600;
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    ret = smu_read_main_power(serial);
    return
end

ret = fourbytestodec_signed(flip(struct(4:7)));
end