#ifndef _HAVERSINE_H_
#define _HAVERSINE_H_

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

const double EARTH_RADIUS_KM = 6371.0; // Earth's radius in kilometers
const double EARTH_RADIUS_M = 6371.0 * 1000.0;

double haversineDistance(double lat1, double lon1, double lat2, double lon2);


#endif // _HAVERSINE_H_