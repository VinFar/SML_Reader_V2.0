t = datetime('now')

sec_tens = floor(t.Second/10)
sec_ones = floor(t.Second - sec_tens*10)

min_tens = floor(t.Minute/10)
min_ones = floor(t.Minute - min_tens*10)

hr_tens = floor(t.Hour/10)
hr_ones = floor(t.Hour - hr_tens*10)

day_tens = floor(t.Day/10)
day_ones = floor(t.Day - day_tens*10)

month_tens = floor(t.Month/10)
month_ones = floor(t.Month - month_tens*10)

year_thousands = floor(t.Year/1000)
year_hundreds = floor((t.Year - year_thousands*1000)/100)
year_tens = floor((t.Year - year_thousands*1000 - year_hundreds*100)/10)
year_ones = floor(t.Year - year_thousands*1000 - year_hundreds*100 - year_tens*10)


sec_ones_str = dec2bin(sec_ones)
sec_tens_str = dec2bin(sec_tens)

min_ones_str = dec2bin(min_ones)
min_tens_str = dec2bin(min_tens)

hr_ones_str = dec2bin(hr_ones)
hr_tens_str = dec2bin(hr_tens)

bcd_TR = dec2bin(sec_ones + sec_tens*2^4 + min_ones*2^8 + min_tens*2^12 + hr_ones*2^16 + hr_tens*2^20)

day_of_week = weekday(t)-1;

if(day_of_week == 0)
    day_of_week=7;
end
day_of_week

bcd_DR = dec2bin(day_ones + day_tens*2^4 + month_ones*2^8 + month_tens*2^12 + day_of_week*2^13 + year_ones*2^16 + year_tens*2^20)

