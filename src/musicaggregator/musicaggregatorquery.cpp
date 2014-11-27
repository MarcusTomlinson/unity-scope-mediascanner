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
#include "musicaggregatorscope.h"
#include "../utils/searchreceiver.h"
#include "../utils/i18n.h"
#include <memory>
#include <functional>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/Location.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>

using namespace unity::scopes;

static const char MYMUSIC_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "horizontal-list",
    "card-size": "small"
  },
  "components": {
    "title": "title",
    "art":  "art",
    "subtitle": "artist"
  }
}
)";

static const char MYMUSIC_SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-layout" : "horizontal",
    "card-size": "large"
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
        "card-size": "large",
        "card-layout" : "horizontal"
    }
}
)";

static const char GROOVESHARK_SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-layout" : "horizontal",
    "card-size": "large"
  },
  "components": {
    "title": "title",
    "art":  "art",
    "subtitle": "artist"
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
        "card-size": "medium"
    }
}
)";

static const char SEVENDIGITAL_SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-layout" : "horizontal",
    "card-size": "large"
  },
  "components": {
        "subtitle": "subtitle",
        "attributes": "attributes",
        "art": "art",
        "title": "title"
  }
}
)";

static const char SOUNDCLOUD_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "large",
    "overlay": true,
    "card-background": "color:///#000000"
  },
  "components": {
    "title": "title",
    "art" : {
      "field": "waveform",
      "aspect-ratio": 4.0
    },
    "subtitle": "username",
    "mascot": "art",
    "attributes": {
      "field": "attributes",
      "max-count": 3
    }
  }
}
)";

static const char SOUNDCLOUD_SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "large",
    "card-layout": "horizontal"
  },
  "components": {
    "title": "title",
    "mascot":  "art",
    "subtitle": "address",
    "attributes": "attributes"
  }
}
)";

static const char SONGKICK_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "large",
    "card-layout": "horizontal"
  },
  "components": {
    "title": "title",
    "art": "art",
    "subtitle": "subtitle"
  }
}
)";

static const char SONGKICK_SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "large",
    "card-layout": "horizontal"
  },
  "components": {
    "title": "title",
    "mascot":  "art",
    "subtitle": "address",
    "attributes": "attributes"
  }
}
)";

static char YOUTUBE_SURFACING_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "medium"
  },
  "components": {
    "title": "title",
    "subtitle": "subtitle",
    "art":  {
      "field": "art",
      "aspect-ratio": 1.5
    }
  }
}
)";

static char YOUTUBE_SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "medium",
    "card-layout": "horizontal"
  },
  "components": {
    "title": "title",
    "subtitle": "subtitle",
    "art":  {
      "field": "art",
      "aspect-ratio": 1.5
    }
  }
}
)";

MusicAggregatorQuery::MusicAggregatorQuery(CannedQuery const& query, SearchMetadata const& hints,
        ScopeProxy local_scope,
        ScopeProxy const& grooveshark_scope,
        ScopeProxy const& soundcloud_scope,
        ScopeProxy const& sevendigital_scope,
        ScopeProxy const& songkick_scope,
        ScopeProxy const& youtube_scope
        ) :
    SearchQueryBase(query, hints),
    local_scope(local_scope),
    grooveshark_scope(grooveshark_scope),
    soundcloud_scope(soundcloud_scope),
    sevendigital_scope(sevendigital_scope),
    songkick_scope(songkick_scope),
    youtube_scope(youtube_scope)
{
}

MusicAggregatorQuery::~MusicAggregatorQuery() {
}

void MusicAggregatorQuery::cancelled() {
}

void MusicAggregatorQuery::run(unity::scopes::SearchReplyProxy const& parent_reply)
{
    std::vector<utility::BufferedResultForwarder::SPtr> replies;
    std::vector<unity::scopes::ScopeProxy> scopes;
    const std::string department_id = "aggregated:musicaggregator";

    const CannedQuery mymusic_query(MusicAggregatorScope::LOCALSCOPE, query().query_string(), "");
    const CannedQuery sevendigital_query(MusicAggregatorScope::SEVENDIGITAL, query().query_string(), "newreleases");
    const CannedQuery soundcloud_query(MusicAggregatorScope::SOUNDCLOUD, query().query_string(), "");
    const CannedQuery songkick_query(MusicAggregatorScope::SONGKICK, query().query_string(), "");
    const CannedQuery grooveshark_query(MusicAggregatorScope::GROOVESHARKSCOPE, query().query_string(), "");
    const CannedQuery youtube_query(MusicAggregatorScope::YOUTUBE, query().query_string(), department_id);

    const bool empty_search = query().query_string().empty();

    //
    // register categories
    auto mymusic_cat = empty_search ? parent_reply->register_category("mymusic", _("My Music"), "",
            mymusic_query, CategoryRenderer(MYMUSIC_CATEGORY_DEFINITION))
        : parent_reply->register_category("mymusic", _("My Music"), "", CategoryRenderer(MYMUSIC_SEARCH_CATEGORY_DEFINITION));
    auto sevendigital_cat = empty_search ? parent_reply->register_category("7digital", _("New albums from 7digital"), "",
            sevendigital_query, CategoryRenderer(SEVENDIGITAL_CATEGORY_DEFINITION))
        : parent_reply->register_category("7digital", _("7digital"), "", CategoryRenderer(SEVENDIGITAL_SEARCH_CATEGORY_DEFINITION));
    auto soundcloud_cat = empty_search ? parent_reply->register_category("soundcloud", _("Popular tracks on SoundCloud"), "",
            soundcloud_query, CategoryRenderer(SOUNDCLOUD_CATEGORY_DEFINITION))
        : parent_reply->register_category("soundcloud", _("SoundCloud"), "", CategoryRenderer(SOUNDCLOUD_SEARCH_CATEGORY_DEFINITION));
    auto songkick_cat = empty_search ? parent_reply->register_category("songkick", _("Nearby Events on Songkick"), "",
            songkick_query, CategoryRenderer(SONGKICK_CATEGORY_DEFINITION))
        : parent_reply->register_category("songkick", _("Songkick"), "", CategoryRenderer(SONGKICK_SEARCH_CATEGORY_DEFINITION));
    auto grooveshark_cat = empty_search ? parent_reply->register_category("grooveshark", _("Popular tracks on Grooveshark"), "",
            grooveshark_query, CategoryRenderer(GROOVESHARK_CATEGORY_DEFINITION))
        : parent_reply->register_category("grooveshark", _("Grooveshark"), "", CategoryRenderer(GROOVESHARK_SEARCH_CATEGORY_DEFINITION));
    auto youtube_cat = empty_search ? parent_reply->register_category("youtube", _("Popular tracks on Youtube"), "",
                youtube_query, CategoryRenderer(YOUTUBE_SURFACING_CATEGORY_DEFINITION))
            : parent_reply->register_category("youtube", _("Youtube"), "", youtube_query, CategoryRenderer(YOUTUBE_SEARCH_CATEGORY_DEFINITION));


    std::shared_ptr<SearchReceiver> previous;
    if (grooveshark_scope)
    {
        scopes.push_back(grooveshark_scope);
        auto reply = std::make_shared<SearchReceiver>(grooveshark_cat, parent_reply, previous, [](CategorisedResult const& result) -> bool {
                return result.category()->id() != "cat_0";
            }
        );
        replies.push_back(reply);
        previous = reply;
    }
    if (songkick_scope)
    {
        scopes.push_back(songkick_scope);
        auto reply = std::make_shared<SearchReceiver>(songkick_cat, parent_reply, previous, [](CategorisedResult const& result) -> bool {
                    return result.category()->id() != "noloc";
                }
            );
        replies.push_back(reply);
        previous = reply;
    }
    if (soundcloud_scope)
    {
        scopes.push_back(soundcloud_scope);
        auto reply = std::make_shared<SearchReceiver>(soundcloud_cat, parent_reply, previous, [](CategorisedResult const& result) -> bool {
                return result.category()->id() != "soundcloud_login_nag";
                }
        );
        replies.push_back(reply);
        previous = reply;
    }
    if (sevendigital_scope)
    {
        scopes.push_back(sevendigital_scope);
        auto reply = std::make_shared<SearchReceiver>(sevendigital_cat, parent_reply, previous);
        replies.push_back(reply);
        previous = reply;
    }
    if (youtube_scope)
    {
        scopes.push_back(youtube_scope);
        auto reply = std::make_shared<SearchReceiver>(youtube_cat, parent_reply, previous, [](CategorisedResult const& result) -> bool {
                return !result["musicaggregation"].is_null();
            }
        );
        replies.push_back(reply);
        previous = reply;
    }

    // local scope
    {
        scopes.push_back(local_scope);
        auto local_reply = std::make_shared<SearchReceiver>(mymusic_cat, parent_reply, previous);
        replies.push_back(local_reply);
    }

    // dispatch search to subscopes
    for (unsigned int i = 0; i < replies.size(); ++i)
    {
        std::string dept;
        SearchMetadata metadata(search_metadata());
        if (scopes[i] == sevendigital_scope)
        {
            if (empty_search)
            {
                dept = "newreleases";
            }
            metadata.set_cardinality(2);
        }
        else if (scopes[i] == local_scope)
        {
            dept = ""; // artists
        }
        else if (scopes[i] == grooveshark_scope)
        {
            if (empty_search)
            {
                metadata.set_cardinality(3);
            }
        }
        else if (scopes[i] == soundcloud_scope)
        {
            if (empty_search)
            {
                metadata.set_cardinality(3);
            }
        }
        else if (scopes[i] == songkick_scope)
        {
            if (empty_search)
            {
                metadata.set_cardinality(2);
            }
        }
        else if (scopes[i] == youtube_scope)
        {
            if (empty_search)
            {
                metadata.set_cardinality(2);
            }
            dept = department_id;
        }

        // Don't send location data to scopes that don't need it.
        if (scopes[i] != songkick_scope)
        {
            metadata.set_location(Location(0, 0));
        }

        subsearch(scopes[i], query().query_string(), dept, FilterState(), metadata, replies[i]);
    }
}
