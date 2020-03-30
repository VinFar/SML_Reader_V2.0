%% zoomPlot     add inlaid plot to current figure
%       [p,z]   = zoomPlot(x,y,xbounds,pos,vertex) where: 
%       x,y     = vectors being plotted
%       xbounds = [x1 x2] specifies the zoom indices, where x1 is the 
%               first x value displayed in the zoom plot and x2 is the last.
%       pos     = [left, bottom, width, height] specifies the location and
%               size of the side of the zoom box, relative to the lower-left
%               corner of the Figure window, in normalized units where (0,0)
%               is the lower-left corner and (1.0,1.0) is the upper-right.
% {opt} vertex  = toggles connecting lines corresponding to vertices, where 1 
%               corresponds to the top left vertex and continuing clockwise, 
%               4 corresponds to the bottom right vertex. All 4 vertices can 
%               be included.
% {opt} p       = axes handle for larger plot
% {opt} z       = axes handle for zoom plot
% 
% Note: place title, labels, and legend BEFORE placing zoom plot,
%     otherwise zoomPlot returns the handle of the original axes (p).
%     Change title using p.Title.String = 'insert title here'
% 
% Kelsey Bower (kelsey.bower@case.edu) 10/20/15
 

function [p z] = zoomPlot(x,y,yp2,yp3,xbounds,pos,vertex,mean)
% Please retain the following:
% 
% Original Author: 
% Kelsey Bower, kelsey.bower@case.edu


% Get current axis position and limits
p = gca;

% Calculate x,y points of zoomPlot
x1 = (pos(1)-p.Position(1))/p.Position(3)*diff(p.XLim)+p.XLim(1);
x2 = (pos(1)+pos(3)-p.Position(1))/p.Position(3)*diff(p.XLim)+(p.XLim(1));
y1 = (pos(2)-p.Position(2))/p.Position(4)*diff(p.YLim)+p.YLim(1);
y2 = ((pos(2)+pos(4)-p.Position(2))/p.Position(4))*diff(p.YLim)+p.YLim(1);

% Plot lines connecting zoomPlot to original plot points
index = find(x>=xbounds(1) & x<=xbounds(2)); % Find indexes of points in zoomPlot
xlimits = xlim();
ylimits2 = ylim();
axes;
rectangle('Position',[datenum(xbounds(1)) min(y(index)) datenum(diff(xbounds)) max(y(index))-min(y(index))]);
xlim(datenum(xlimits))
ylim(ylimits2);
axis off;
hold on
if any(vertex==1)
    plot(datenum([xbounds(1) x1]), [max(y(index)) y2], 'k'); % Line to vertex 1
end
if any(vertex==2)
    plot(datenum([xbounds(2) x2]), [max(y(index)) y2], 'k'); % Line to vertex 2
end
if any(vertex==3)
    plot(datenum([xbounds(2) x2]), [min(y(index)) y1], 'k'); % Line to vertex 4
end
if any(vertex==4)
    plot(datenum([xbounds(1) x1]), [min(y(index)) y1], 'k'); % Line to vertex 3
end

% Plot zoomPlot and change axis
z = axes('position',pos);
box on 
ar1 = area(x,movmean(y,mean));
hold on
ar1.FaceColor =[0.8500 0.3250 0.0980];
ar1.FaceAlpha = 0.7
ar2 = area(x,movmean(yp2,mean));
ar2.FaceColor =  [0 0.4470 0.7410];
ar2.FaceAlpha = 0.7
ar3 = area(x,movmean(-yp3,mean));
ar3.FaceColor =[0.4660 0.6740 0.1880];
ar3.FaceAlpha = 0.7
grid on
grid minor
xlim([(xbounds(1)) (xbounds(2))])
ylim([min(y(index))*1.1 max(y(index))]);
ylabel('Power in W');
end