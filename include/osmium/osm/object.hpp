#ifndef OSMIUM_OSM_OBJECT_HPP
#define OSMIUM_OSM_OBJECT_HPP

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

#include <cstdint>
#include <cstdlib>

#include <boost/operators.hpp>

#include <osmium/osm/location.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/timestamp.hpp>
#include <osmium/osm/types.hpp>

namespace osmium {

    namespace memory {
        template <class T> class ObjectBuilder;
    }

    /**
     * OSM Object (Node, Way, or Relation).
     */
    class Object : public osmium::memory::Item, boost::less_than_comparable<Object> {

        object_id_type      m_id;
        bool                m_deleted : 1;
        object_version_type m_version : 31;
        osmium::Timestamp   m_timestamp;
        user_id_type        m_uid;
        changeset_id_type   m_changeset;

        size_t sizeof_object() const {
            return sizeof(Object) + (type() == item_type::node ? sizeof(osmium::Location) : 0);
        }

        unsigned char* user_position() {
            return data() + sizeof_object();
        }

        const unsigned char* user_position() const {
            return data() + sizeof_object();
        }

        string_size_type user_length() const {
            return *reinterpret_cast<const string_size_type*>(user_position());
        }

        unsigned char* subitems_position() {
            return user_position() + osmium::memory::padded_length(sizeof(string_size_type) + user_length());
        }

        const unsigned char* subitems_position() const {
            return user_position() + osmium::memory::padded_length(sizeof(string_size_type) + user_length());
        }

    protected:

        Object(osmium::memory::item_size_type size, osmium::item_type type) :
            Item(size, type),
            m_id(0),
            m_deleted(false),
            m_version(0),
            m_timestamp(),
            m_uid(0),
            m_changeset(0) {
        }

        template <class T>
        T& subitem_of_type() {
            for (iterator it = begin(); it != end(); ++it) {
                if (it->type() == T::itemtype) {
                    return reinterpret_cast<T&>(*it);
                }
            }

            static T subitem;
            return subitem;
        }

        template <class T>
        const T& subitem_of_type() const {
            for (const_iterator it = cbegin(); it != cend(); ++it) {
                if (it->type() == T::itemtype) {
                    return reinterpret_cast<const T&>(*it);
                }
            }

            static const T subitem;
            return subitem;
        }

    public:

        /// Get ID of this object.
        object_id_type id() const {
            return m_id;
        }

        /// Get absolute value of the ID of this object.
        unsigned_object_id_type positive_id() const {
            return std::abs(m_id);
        }

        /**
         * Set ID of this object.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& id(object_id_type id) {
            m_id = id;
            return *this;
        }

        /**
         * Set ID of this object.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& id(const char* id) {
            return this->id(osmium::string_to_object_id(id));
        }

        /// Is this object marked as deleted?
        bool deleted() const {
            return m_deleted;
        }

        /// Is this object marked visible (ie not deleted)?
        bool visible() const {
            return !deleted();
        }

        /**
         * Mark this object as deleted (or not).
         *
         * @return Reference to object to make calls chainable.
         */
        Object& deleted(bool deleted) {
            m_deleted = deleted;
            return *this;
        }

        /**
         * Mark this object as visible (ie not deleted) (or not).
         *
         * @return Reference to object to make calls chainable.
         */
        Object& visible(bool visible) {
            m_deleted = !visible;
            return *this;
        }

        /**
         * Mark this object as visible (ie not deleted) or deleted.
         *
         * @param visible Either "true" or "false"
         * @return Reference to object to make calls chainable.
         */
        Object& visible(const char* visible) {
            if (!strcmp("true", visible)) {
                this->visible(true);
            } else if (!strcmp("false", visible)) {
                this->visible(false);
            } else {
                throw std::runtime_error("Unknown value for visible attribute (allowed is 'true' or 'false')");
            }
            return *this;
        }

        /// Get version of this object.
        object_version_type version() const {
            return m_version;
        }

        /**
         * Set object version.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& version(object_version_type version) {
            m_version = version;
            return *this;
        }

        /**
         * Set object version.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& version(const char* version) {
            return this->version(string_to_object_version(version));
        }

        /// Get changeset id of this object.
        changeset_id_type changeset() const {
            return m_changeset;
        }

        /**
         * Set changeset id of this object.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& changeset(changeset_id_type changeset) {
            m_changeset = changeset;
            return *this;
        }

        /**
         * Set changeset id of this object.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& changeset(const char* changeset) {
            return this->changeset(string_to_changeset_id(changeset));
        }

        /// Get user id of this object.
        user_id_type uid() const {
            return m_uid;
        }

        /**
         * Set user id of this object.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& uid(user_id_type uid) {
            m_uid = uid;
            return *this;
        }

        /**
         * Set user id of this object.
         * Sets uid to 0 (anonymous) if the given uid is smaller than 0.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& uid_from_signed(int32_t uid) {
            m_uid = uid < 0 ? 0 : uid;
            return *this;
        }

        /**
         * Set user id of this object.
         *
         * @return Reference to object to make calls chainable.
         */
        Object& uid(const char* uid) {
            return this->uid_from_signed(string_to_user_id(uid));
        }

        /// Is this user anonymous?
        bool user_is_anonymous() const {
            return m_uid == 0;
        }

        /// Get timestamp when this object last changed.
        osmium::Timestamp timestamp() const {
            return m_timestamp;
        }

        /**
         * Set the timestamp when this object last changed.
         *
         * @param timestamp Timestamp
         * @return Reference to object to make calls chainable.
         */
        Object& timestamp(const osmium::Timestamp timestamp) {
            m_timestamp = timestamp;
            return *this;
        }

        /// Get user name for this object.
        const char* user() const {
            return reinterpret_cast<const char*>(data() + sizeof_object() + sizeof(string_size_type));
        }

        /// Get the list of tags for this object.
        TagList& tags() {
            return subitem_of_type<TagList>();
        }

        /// Get the list of tags for this object.
        const TagList& tags() const {
            return subitem_of_type<const TagList>();
        }

        /**
         * Set named attribute.
         *
         * @param attr Name of the attribute (must be one of "id", "version", "changeset", "timestamp", "uid", "visible")
         * @param value Value of the attribute
         */
        void set_attribute(const char* attr, const char* value) {
            if (!strcmp(attr, "id")) {
                id(value);
            } else if (!strcmp(attr, "version")) {
                version(value);
            } else if (!strcmp(attr, "changeset")) {
                changeset(value);
            } else if (!strcmp(attr, "timestamp")) {
                timestamp(osmium::Timestamp(value));
            } else if (!strcmp(attr, "uid")) {
                uid(value);
            } else if (!strcmp(attr, "visible")) {
                visible(value);
            }
        }

        typedef osmium::memory::CollectionIterator<Item> iterator;
        typedef osmium::memory::CollectionIterator<const Item> const_iterator;

        iterator begin() {
            return iterator(subitems_position());
        }

        iterator end() {
            return iterator(data() + padded_size());
        }

        const_iterator cbegin() const {
            return const_iterator(subitems_position());
        }

        const_iterator cend() const {
            return const_iterator(data() + padded_size());
        }

        const_iterator begin() const {
            return cbegin();
        }

        const_iterator end() const {
            return cend();
        }

    }; // class Object

    static_assert(sizeof(Object) % osmium::memory::align_bytes == 0, "Class osmium::Object has wrong size to be aligned properly!");

} // namespace osmium

#endif // OSMIUM_OSM_OBJECT_HPP
