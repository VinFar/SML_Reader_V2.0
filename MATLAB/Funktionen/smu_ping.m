function smu_ping(serial)
%reads the temperature of the incubation chamber
%upper command: 1
%lower command: 0

struct = create_cmd_struct(1,0,0,0);
send_cmd_struct(serial,struct);
struct = receive_ack_struct(serial);
if(struct(2)~=70)
    smu_ping(serial);
    return
end
end