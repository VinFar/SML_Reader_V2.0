function [ret] = begin_STM32F7()
%opens the serial port to the MCU on the main PCD and returns its serial
%device


delete(instrfindall) % closes all serial ports

[s, out] = dos('vol');
sc = strsplit(out,'\n');
VolLbl = sc{2}(end-8:end);     % ‘VolLbl’ is a (1x9) char array

if(strcmp(VolLbl,'E6D7-27B9'))
    disp('MATLAB is running on Vincents Desktop')
    com = 'COM7';
else
    disp('MATLAB is running on Lab Laptop')
    com= 'COM3';
end

stm32=serial(com,'BaudRate',1000000); %Konfiguriert Serial Port
fopen(stm32);
stm32.Timeout = 1;


t = timerInBackground;
t.UserData = stm32;
% start(t)

% stm32.Terminator = 0;
read_pressure_port21(stm32);
read_pressure_port21(stm32);
read_pressure_port21(stm32);
read_pressure_port21(stm32);

ret = stm32;
end

