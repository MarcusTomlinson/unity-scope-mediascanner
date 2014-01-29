/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *
 */

#include "musicaggregatorscope.h"
#include "musicaggregatorquery.h"
#include <unity/scopes/Registry.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/CategoryRenderer.h>

using namespace unity::scopes;

const char *LOCALSCOPE = "mediascanner-music";
const char *ONLINESCOPE = "online-music";

int MusicAggregatorScope::start(std::string const&, unity::scopes::RegistryProxy const& registry) {
    CategoryRenderer basic;
    local_scope = registry->get_metadata(LOCALSCOPE).proxy();
    online_scope = registry->get_metadata(ONLINESCOPE).proxy();
    return VERSION;
}

void MusicAggregatorScope::stop() {
}

unity::scopes::QueryBase::UPtr MusicAggregatorScope::create_query(std::string const& q,
        unity::scopes::VariantMap const&) {
    unity::scopes::QueryBase::UPtr query(new MusicAggregatorQuery(q, local_scope, online_scope));
    return query;
}

QueryBase::UPtr MusicAggregatorScope::preview(Result const& /*result*/, VariantMap const& /*hints*/) {
    return nullptr;
}

#define EXPORT __attribute__ ((visibility ("default")))

extern "C"
{

    EXPORT
    unity::scopes::ScopeBase*
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_CREATE_FUNCTION()
    {
        return new MusicAggregatorScope();
    }

    EXPORT
    void
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_DESTROY_FUNCTION(unity::scopes::ScopeBase* scope_base)
    {
        delete scope_base;
    }

}
