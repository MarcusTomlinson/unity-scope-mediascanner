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

#include "musicaggregatorquery.h"
#include "categoryadder.h"
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/Query.h>
#include <unity/scopes/SearchReply.h>

#include <unity/scopes/SearchListener.h>

using namespace unity::scopes;

MusicAggregatorQuery::MusicAggregatorQuery(std::string const& query, ScopeProxy local_scope, ScopeProxy online_scope) :
query(query), local_scope(local_scope), online_scope(online_scope) {
}

MusicAggregatorQuery::~MusicAggregatorQuery() {
}

void MusicAggregatorQuery::cancelled() {
}

void MusicAggregatorQuery::run(unity::scopes::SearchReplyProxy const& parent_reply) {
    CategoryRenderer r;
    auto catloc = parent_reply->register_category("localmusic", "Local music", "icon", r);
    auto catonl = parent_reply->register_category("onlinemusic", "Online music", "icon", r);
    SearchListener::SPtr local_reply(new CategoryAdder(catloc, parent_reply));
    create_subquery(local_scope, query, VariantMap(), local_reply);
    SearchListener::SPtr online_reply(new CategoryAdder(catonl, parent_reply));
    create_subquery(online_scope, query, VariantMap(), online_reply);
}