#if !defined(GEOUTIL_HPP)
#define GEOUTIL_HPP
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <math.h>

#include "mslogger.hpp"

#define EPSILON 0.4
#define LARGE_FLOAT 99999.9f

struct GeoPoint
{
    GeoPoint():x_(0),y_(0){}
    GeoPoint(float x, float y) : x_(x),y_(y){}
    GeoPoint(std::string x, std::string y) : x_(atof(x.c_str())),y_(atof(y.c_str())){}
    GeoPoint(char *x, char *y) : x_(atof(x)),y_(atof(y)){}
    GeoPoint(const char *x, const char *y) : x_(atof(x)),y_(atof(y)){}
    GeoPoint(std::string xy)
    {
        size_t commaIndex = xy.find(',');
        x_ = atof(xy.substr(0,commaIndex).c_str());
        y_ = atof(xy.substr(commaIndex+1,xy.length()).c_str());
    }
    bool operator==(const GeoPoint& other)
    {
        if(x_ == other.x_ && y_ == other.y_) return true;
        return false;
    }
    bool operator!=(const GeoPoint& other)
    {
        return !(*this == other);
    }

    float x_,y_;
};

class GeoUtil
{
public:
    GeoUtil(){}
    ~GeoUtil(){}
    void set_station(float latitude, float longitude) {station_ = GeoPoint(latitude,longitude);}
    void set_station(std::string latitude, std::string longitude) {set_station(atof(latitude.c_str()),atof(longitude.c_str()));}
    void set_station(GeoPoint s) {station_ = s;}
    void set_station(std::string s){station_ = GeoPoint(s);}
    void set_alertArea(std::string areaPolygon);
    bool is_inside();

private:
    GeoPoint station_;
    std::vector<GeoPoint> alertArea_;
 
    bool is_online(float x2, float y2, float x1, float y1, float x3, float y3);
    
    bool are_intersecting(
        float v1x1, float v1y1, float v1x2, float v1y2, 
        float v2x1, float v2y1, float v2x2, float v2y2
    ); 
};

#endif // GEOUTIL_HPP