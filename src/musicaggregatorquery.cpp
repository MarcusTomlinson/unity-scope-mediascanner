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
 *             Pawel Stolowski <pawel.stolowski@canonical.com>
 *
 */

#include <config.h>
#include "musicaggregatorquery.h"
#include "resultforwarder.h"
#include "onlinemusicresultforwarder.h"
#include "notify-strategy.h"
#include "i18n.h"
#include <memory>
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>

using namespace unity::scopes;

static const char MYMUSIC_CATEGORYDEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "carousel",
    "overlay": true,
    "card-size": "medium"
  },
  "components": {
    "title": "title",
    "art":  "art",
    "subtitle": "artist"
  }
}
)";

static const char GROOVESHARK_CATEGORY_DEFINITION[] = R"(
{
    "schema-version": 1,
    "components": {
        "subtitle": "artist",
        "art": "art",
        "title": "title"
    },
    "template": {
        "category-layout": "grid",
        "card-size": "medium"
    }
}
)";

static const char SEVENDIGITAL_CATEGORY_DEFINITION[] = R"(
{
    "schema-version": 1,
    "components": {
        "subtitle": "subtitle",
        "attributes": "attributes",
        "art": "art",
        "title": "title"
    },
    "template":
    {
        "category-layout": "grid",
        "card-size": "small",
        "collapsed-rows": 0
    }
}
)";

const std::string MusicAggregatorQuery::grooveshark_songs_category_id = "cat_0";

MusicAggregatorQuery::MusicAggregatorQuery(CannedQuery const& query, SearchMetadata const& hints,
        ScopeProxy local_scope,
        unity::scopes::ScopeProxy const& grooveshark_scope,
        unity::scopes::ScopeProxy const& soundcloud_scope,
        unity::scopes::ScopeProxy const& sevendigital_scope
        ) :
    SearchQueryBase(query, hints),
    local_scope(local_scope),
    grooveshark_scope(grooveshark_scope),
    soundcloud_scope(soundcloud_scope),
    sevendigital_scope(sevendigital_scope)
{
}

MusicAggregatorQuery::~MusicAggregatorQuery() {
}

void MusicAggregatorQuery::cancelled() {
}

void MusicAggregatorQuery::run(unity::scopes::SearchReplyProxy const& parent_reply) {
    std::shared_ptr<ResultForwarder> local_reply(new ResultForwarder(parent_reply));

    std::vector<unity::scopes::ScopeProxy> scopes({local_scope});
    if (grooveshark_scope)
        scopes.push_back(grooveshark_scope);
    if (soundcloud_scope)
        scopes.push_back(soundcloud_scope);
    if (sevendigital_scope)
        scopes.push_back(sevendigital_scope);

    std::vector<std::shared_ptr<ResultForwarder>> replies({local_reply});

    auto mymusic_cat = parent_reply->register_category("mymusic", _("My Music"), "", CategoryRenderer(MYMUSIC_CATEGORYDEFINITION));
    auto grooveshark_cat = parent_reply->register_category("grooveshark", _("Grooveshark"), "", CategoryRenderer(GROOVESHARK_CATEGORY_DEFINITION));
    //auto soundcloud_cat = parent_reply->register_category("soundcloud", _("Soundcloud"), "", CategoryRenderer(SOUNDCLOUD_CATEGORY_DEFINITION));
    auto sevendigital_cat = parent_reply->register_category("7digital", _("7digital"), "", CategoryRenderer(SEVENDIGITAL_CATEGORY_DEFINITION));

    // create and chain buffered result forwarders to enforce proper order of categories
    for (unsigned int i = 0; i < scopes.size(); ++i)
    {
        std::shared_ptr<OnlineMusicResultForwarder> reply;
        if (scopes[i] == grooveshark_scope)
        {
            reply = std::make_shared<OnlineMusicResultForwarder>(parent_reply, [this, grooveshark_cat](CategorisedResult& res) -> bool {
                    if (res.category()->id() == grooveshark_songs_category_id)
                    {
                        res.set_category(grooveshark_cat);
                        return true;
                    }
                    return false;
                });
        }
        else
        {
            reply = std::make_shared<OnlineMusicResultForwarder>(parent_reply, [](CategorisedResult& res) -> bool { return true; });
        }
        replies.push_back(reply);
        replies[i-1]->add_observer(reply);
    }

    // dispatch search to subscopes
    for (unsigned int i = 0; i < replies.size(); ++i)
    {
        std::string dept;
        if (scopes[i] == sevendigital_scope)
        {
            dept = "newreleases";
        }
        else if (scopes[i] == local_scope)
        {
            dept = "tracks";
        }
        subsearch(scopes[i], query().query_string(), dept, FilterState(), replies[i]);
    }
}
