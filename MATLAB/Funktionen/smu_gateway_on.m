function smu_gateway_on(serial)
%reads the temperature of the incubation chamber
%upper command: 1
%lower command: 0

struct = create_cmd_struct(7,0,0,0);
send_cmd_struct(serial,struct);
serial.BaudRate=9600;
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    smu_gateway_on(serial);
    return
end
end