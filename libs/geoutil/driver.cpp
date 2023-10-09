#include "geoutil.hpp"
#include <iostream>
#include <vector>

int main()
{
    std::vector<GeoPoint> points;
    points.push_back(GeoPoint(43.4887976,-80.9314785));
    points.push_back(GeoPoint(43.6078687,-80.3194287));
    points.push_back(GeoPoint(43.4198664,-82.2591451));
    points.push_back(GeoPoint(42.4473599,-81.0111467));
    points.push_back(GeoPoint(43.3415702,-80.2241216));
    points.push_back(GeoPoint(43.9881932,-80.7905499));
    points.push_back(GeoPoint(43.7335073,-81.6228752));
    points.push_back(GeoPoint(43.0441965,-81.5402841));
    points.push_back(GeoPoint(42.3841111,-81.8361111));
    points.push_back(GeoPoint(54.2956111,-116.552611)); // inside
    points.push_back(GeoPoint(54.7619111,-115.986411)); // on-line

std::string polygon = "54.8067,-115.9811 54.7619,-115.9864 54.7626,-115.9676 54.5,-115.9625 54.4127,-115.9667 54.4114,-116.0273 54.4145,-116.1014 54.3264,-116.1004 54.3263,-116.2514 54.2948,-116.2507 54.256,-116.2497 54.226,-116.2344 54.1879,-116.4279 54.1086,-116.628 54.1027,-116.6915 54.0964,-116.6994 54.0689,-116.7337 54.0401,-116.7339 53.9759,-116.8345 53.9932,-116.9497 53.9528,-117.0632 53.9821,-117.2094 53.9705,-117.2349 53.9725,-117.2856 54.016,-117.313 54.008,-117.3559 54.0246,-117.4971 54.0456,-117.5201 54.0527,-117.5282 54.0883,-117.5281 54.5875,-117.5267 54.5875,-116.745 54.7256,-116.7526 54.8503,-116.7449 54.8504,-115.9848 54.8067,-115.9811";

    GeoUtil g;
    g.set_alertArea(polygon);
    for(const GeoPoint& point : points)
    {
        g.set_station(point);
        std::cout << point.x_ << "," << point.y_ << " " << g.is_inside() << std::endl;
    }
}