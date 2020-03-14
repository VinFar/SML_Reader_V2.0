stm32 = connect_smu;
% set rtc ideal freq
rtc_ref_freq = 32768
% set datetime when was last RTC sync
last_sync_dt = datetime(2020, 03, 14, 13, 45, 00)
% set reference datetime when RTC was checked
check_ref_dt = datetime('now')
% set RTC datetime when it was checked
check_rtc_dt = datetime(smu_get_unix_time(stm32), 'convertfrom','posixtime')

% calculate all things
check_total_s = (check_ref_dt - last_sync_dt)
rtc_err_s = (check_rtc_dt - check_ref_dt)
rtc_err_ppm = 1E6 * rtc_err_s / check_total_s
rtc_real_freq = rtc_ref_freq + rtc_ref_freq * rtc_err_ppm * 1e-6

if rtc_err_ppm < 0
    calp = 1
else 
    calp = 0
end
calm = calp * 512 - ((2^20) * (rtc_ref_freq - rtc_real_freq) + calp * 512 * (rtc_real_freq - rtc_ref_freq)) / (rtc_ref_freq)
rtc_calib_freq = rtc_real_freq * (1 + ((calp * 512 - calm)/((2^20) + calm - calp * 512)))


fprintf(strcat("Last RTC sync before check: ",datestr(check_total_s,'HH:MM'),"\n"));
fprintf(strcat("RTC is slow/fast for %d seconds" ,datestr(rtc_err_s),"\n"));
fprintf(strcat("RTC frequency error is x ppm: " , num2str(rtc_err_ppm),"\n"));
fprintf(strcat("RTC real frequency is x Hz: " , num2str(rtc_real_freq),"\n"));
fprintf(strcat("Calculated value for RTC_CALR.CALP = " , num2str(calp),"\n"));
fprintf(strcat("Calculated value for RTC_CALR.CALM = " , num2str(calm),"\n"));
fprintf(strcat("RTC calibrated frequency will be x Hz: " , num2str(rtc_calib_freq),"\n"));

