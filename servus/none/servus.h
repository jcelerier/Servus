/* Copyright (c) 2014-2015, Stefan.Eilemann@epfl.ch
 *
 * This file is part of Servus <https://github.com/HBPVIS/Servus>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

namespace servus
{
namespace none
{
class Servus final : public detail::Servus
{
public:
    explicit Servus( const std::string& name ) : detail::Servus( name ) {}
    ~Servus() override {}

    std::string getClassName() const override { return "none"; }

    servus::Result announce( const unsigned short,
                                       const std::string& ) final override
    {
        return servus::Result(
            servus::Result::NOT_SUPPORTED );
    }

    void withdraw() final override {}
    bool isAnnounced() const final override { return false; }

    servus::Result beginBrowsing(
        const servus::Interface ) final override
    {
        return servus::Result(
            servus::Result::NOT_SUPPORTED );
    }

    servus::Result browse( const int32_t ) final override
    {
        return servus::Result(
            servus::Result::NOT_SUPPORTED );
    }

    void endBrowsing() final override {}
    bool isBrowsing() const final override { return false; }
    Strings discover( const servus::Interface, const unsigned ) final override
    {
        return getInstances();
    }

    void _updateRecord() final override {}
};
}
}
