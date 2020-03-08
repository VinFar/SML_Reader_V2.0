function [ret,error] = receive_ack_struct(serial)
%receives the acknowledge frame from the STM32F7
timeout=2000;
i=1;
error=0;

while(serial.TransferStatus ~= 'idle')
    i=i+1;
end
while(~serial.BytesAvailable)
    timeout=timeout-1;
     pause(0.001)
    if(timeout==0)
        ret=zeros(1,12);
        error=1;
        if(serial.BytesAvailable)
            fread(serial,serial.BytesAvailable)
        end
        return
    end
end
ret_size = fread(serial,2)';
ret_size = twobytestodec(flip(ret_size));
while(serial.TransferStatus ~= 'idle')
    i=i+1;
end
if(serial.BytesAvailable > 0)
    ret = fread(serial,serial.BytesAvailable)';
else
     ret=zeros(1,12);
    error=1;
    if(serial.BytesAvailable)
        fread(serial,serial.BytesAvailable);
    end
    return
end
if(length(ret) ~= ret_size-2)
    ret=zeros(1,12);
    error=1;
    if(serial.BytesAvailable)
        fread(serial,serial.BytesAvailable);
    end
    return
end
ret = [ret_size,ret];

crc = crc32(ret(1,1:end-4));
crc_ck = typecast(uint8([ret(1,end-3),ret(1,end-2),ret(1,end-1),ret(1,end)]),'uint32');

if(0)
    ret=zeros(1,12);
    error=1;
    if(serial.BytesAvailable)
        fread(serial,serial.BytesAvailable)
    end
    return
end
if(ret(1,2)~=70)
    error=1;
    ret=zeros(1,12);
    timeout=200;
    while(~serial.BytesAvailable)
        timeout=timeout-1;
%         pause(0.001)
        if(timeout==0)
            if(serial.BytesAvailable)
                fread(serial,serial.BytesAvailable)
            end
            return;
        end
    end
    fread(serial,serial.BytesAvailable);
end
end

