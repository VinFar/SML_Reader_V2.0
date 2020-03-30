function [ fig ] = plot_data(powermain,powerplant,time,start,stop,mean)
fig = figure('units','normalized','outerposition',[0 0 1 1]);
powerdiff = powerplant - powermain;


ar1 = area(time(1,1:end),movmean(powerplant,mean));
hold on
ar1.FaceColor = [0 0.4470 0.7410];
ar1.FaceAlpha = 0.7
ar2 = area(time(1,1:end),movmean(powerdiff,mean));
ar2.FaceColor = [0.8500 0.3250 0.0980];
ar2.FaceAlpha = 0.7
ar3 = area(time(1,1:end),movmean(-powermain,mean));
ar3.FaceColor =[0.4660 0.6740 0.1880];
ar3.FaceAlpha = 0.7
xlim([start,stop]);
ax = ancestor(ar3, 'axes')
ax.YAxis.Exponent = 0


grid on
grid minor
ylabel('Power in W');
xlabel('Time');
title(['Power distribution with ',num2str(mean),' item movmean'])
legend('Power of PV Plant','Power of Consumers','Power total')
[p,z] = zoomPlot(time,powerdiff,powerplant,powermain,[datetime(2020,3,25,16,40,0),datetime(2020,3,25,17,30,0)],[0.6,0.15,0.28,0.33],[1,2,3,4],mean)

% [5 50],[0.33 0.35 0.3 0.55],[1 3]

end

