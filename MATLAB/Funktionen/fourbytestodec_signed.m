function [ret_float] = fourbytestodec_signed(input)
%UNTITLED Summary of this function goes here
%   function for converting 4 bytes into a floating point value
%   input: array of 4 Bytes
ret_float = (input); %// Collected bytes
ret_float = flip(ret_float);
ret_float = uint8(ret_float) ;                      %// cast them to "uint8" if they are not already
ret_float = typecast( ret_float , 'int32_t') ;  %// cast the 4 bytes as a 32 bit float


end

