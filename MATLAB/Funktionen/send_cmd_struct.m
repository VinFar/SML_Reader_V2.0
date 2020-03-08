function send_cmd_struct(serial,vec)
%sends the passed struct to the STM32F7 MCU over usart
i=0;
while(serial.TransferStatus ~= 'idle')
    i=i+1;
end
fwrite(serial,uint8(vec),'sync');
% pause(0.001);
end

