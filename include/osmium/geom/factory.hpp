#ifndef OSMIUM_GEOM_FACTORY_HPP
#define OSMIUM_GEOM_FACTORY_HPP

/*

This file is part of Osmium (http://osmcode.org/osmium).

Copyright 2013 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <exception>
#include <string>

#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>

namespace osmium {

    /**
     * @brief Namespace for everything related to geometry.
     */
    namespace geom {

        struct geometry_error : public std::runtime_error {

            geometry_error(const std::string& what) :
                std::runtime_error(what) {
            }

            geometry_error(const char* what) :
                std::runtime_error(what) {
            }

        };

        /**
         * Abstract base class for geometry factories.
         */
        template <class G, class T>
        class GeometryFactory {

        protected:

            GeometryFactory<G, T>() {
            }

        public:

            typedef typename T::point_type point_type;
            typedef typename T::linestring_type linestring_type;
            typedef typename T::polygon_type polygon_type;

            point_type create_point(const osmium::Location location) {
                if (location) {
                    return static_cast<G*>(this)->make_point(location);
                } else {
                    throw geometry_error("location is undefined");
                }
            }

            point_type create_point(const osmium::Node& node) {
                return create_point(node.location());
            }

            point_type create_point(const osmium::WayNode& way_node) {
                return create_point(way_node.location());
            }

            linestring_type create_linestring(const osmium::WayNodeList& wnl, bool unique=true, bool reverse=false) {
                static_cast<G*>(this)->linestring_start();

                if (unique) {
                    osmium::Location last_location;
                    if (reverse) {
                        for (int i = wnl.size()-1; i >= 0; --i) {
                            if (last_location != wnl[i].location()) {
                                last_location = wnl[i].location();
                                if (last_location) {
                                    static_cast<G*>(this)->linestring_add_location(last_location);
                                } else {
                                    throw geometry_error("location is undefined");
                                }
                            }
                        }
                    } else {
                        for (auto& wn : wnl) {
                            if (last_location != wn.location()) {
                                last_location = wn.location();
                                if (last_location) {
                                    static_cast<G*>(this)->linestring_add_location(last_location);
                                } else {
                                    throw geometry_error("location is undefined");
                                }
                            }
                        }
                    }
                } else {
                    if (reverse) {
                        for (int i = wnl.size()-1; i >= 0; --i) {
                            if (wnl[i].location()) {
                                static_cast<G*>(this)->linestring_add_location(wnl[i].location());
                            } else {
                                throw geometry_error("location is undefined");
                            }
                        }
                    } else {
                        for (auto& wn : wnl) {
                            if (wn.location()) {
                                static_cast<G*>(this)->linestring_add_location(wn.location());
                            } else {
                                throw geometry_error("location is undefined");
                            }
                        }
                    }
                }

                return static_cast<G*>(this)->linestring_finish();
            }

            linestring_type create_linestring(const osmium::Way& way, bool unique=true, bool reverse=false) {
                return create_linestring(way.nodes(), unique, reverse);
            }

        }; // class GeometryFactory

    } // namespace geom

} // namespace osmium

#endif // OSMIUM_GEOM_FACTORY_HPP
