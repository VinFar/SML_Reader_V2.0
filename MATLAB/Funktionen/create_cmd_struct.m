function [ret] = create_cmd_struct(cmd,major,minor,data)
%creates and fills a command structure as passes it back as a return argument
switch(class(data))
    case 'single'
        tmp = typecast(single(data),'uint8');
    case 'uint16'
        tmp = typecast(uint16(data),'uint8');
    case 'uint8'
        tmp = typecast(uint8(data),'uint8');
    case 'uint32'
        tmp = typecast(uint32(data),'uint8');
    case 'double'
        tmp = typecast(single(data),'uint8');
    otherwise
        cmd=0;
        major=0;
        minor=0;
        data=0;
        return;
end

size_data = size(tmp);

vec = zeros(1,11 + size_data(1,2));
vec(1,2) = cmd;
vec(1,1) = 11 + size_data(1,2);
tmp = typecast(uint16(major),'uint8');
vec(1,3:4) = tmp;
tmp = typecast(uint16(minor),'uint8');
vec(1,5:6) = tmp;
tmp = typecast((data),'uint8');
len = size(tmp);
for i=1:1:len(1,2)
    vec(1,i+6) = tmp(1,i);
end
vec(1,end-4) =  234;
tmp = typecast(uint32(crc32(vec(1,1:end-4))),'uint8');
vec(1,end-3:end) = tmp;
ret = vec;
end


