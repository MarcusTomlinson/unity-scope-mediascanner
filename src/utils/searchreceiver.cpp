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
 * Authored by Pawel Stolowski <pawel.stolowski@canonical.com>
 *
 */

#include "searchreceiver.h"
#include <unity/scopes/SearchReply.h>

using namespace unity::scopes;

SearchReceiver::SearchReceiver(Category::SCPtr target_cat, SearchReplyProxy const& upstream, ResultFilterFunction const& result_filter)
    : utility::BufferedResultForwarder(upstream),
    category_(target_cat),
    result_filter_(result_filter)
{
}

SearchReceiver::SearchReceiver(Category::SCPtr target_cat, SearchReplyProxy const& upstream, utility::BufferedResultForwarder::SPtr const& next_forwarder,
        ResultFilterFunction const& result_filter)
    : utility::BufferedResultForwarder(upstream, next_forwarder),
    category_(target_cat),
    result_filter_(result_filter)
{
}

void SearchReceiver::push(CategorisedResult result)
{
    if (result_filter_(result))
    {
        result.set_category(category_);
        upstream()->push(result);
        set_ready();
    }
}
