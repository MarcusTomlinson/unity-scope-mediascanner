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

#ifndef SEARCH_RECEIVER_H
#define SEARCH_RECEIVER_H

#include <unity/scopes/utility/BufferedResultForwarder.h>
#include <unity/scopes/SearchReplyProxyFwd.h>

typedef std::function<bool(const unity::scopes::CategorisedResult& res)> ResultFilterFunction;

class SearchReceiver : public unity::scopes::utility::BufferedResultForwarder
{
public:
    SearchReceiver(unity::scopes::Category::SCPtr target_cat, unity::scopes::SearchReplyProxy const& upstream, ResultFilterFunction const& result_filter = [](const
                unity::scopes::CategorisedResult&) { return true; });

    SearchReceiver(unity::scopes::Category::SCPtr target_cat, unity::scopes::SearchReplyProxy const& upstream,
            unity::scopes::utility::BufferedResultForwarder::SPtr const& next_forwarder, ResultFilterFunction const& result_filter = [](const
                unity::scopes::CategorisedResult&) { return true; });

    void push(unity::scopes::CategorisedResult result) override;

private:
    unity::scopes::Category::SCPtr category_;
    ResultFilterFunction result_filter_;
};

#endif
