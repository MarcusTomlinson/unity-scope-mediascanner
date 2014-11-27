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

#include <config.h>

#include <cstdio>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>

#include "../utils/i18n.h"
#include "videoaggregatorquery.h"
#include "../utils/searchreceiver.h"

using namespace unity::scopes;

static char SURFACING_CATEGORY_DEFINITION[] = R"(
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

static char SEARCH_CATEGORY_DEFINITION[] = R"(
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

VideoAggregatorQuery::VideoAggregatorQuery(CannedQuery const& query, SearchMetadata const& hints, std::vector<ScopeMetadata> subscopes) :
    SearchQueryBase(query, hints),
    subscopes(subscopes) {
}

VideoAggregatorQuery::~VideoAggregatorQuery() {
}

void VideoAggregatorQuery::cancelled() {
}

void VideoAggregatorQuery::run(unity::scopes::SearchReplyProxy const& parent_reply) {
    const std::string query_string = query().query_string();
    const bool surfacing = query_string.empty();
    const std::string department_id = "aggregated:videoaggregator";
    const FilterState filter_state;
    const VariantMap config = settings();

    std::shared_ptr<SearchReceiver> fwd;

    std::vector<std::shared_ptr<SearchReceiver>> forwarders(subscopes.size());
    // Create forwarders for all scopes
    for (int i = subscopes.size() - 1; i>=0; i--) {
        const auto &metadata = subscopes[i];
        const std::string scope_id = metadata.scope_id();
        try {
            if (!config.at(scope_id).get_bool()) {
                continue;
            }
        } catch (const std::exception &e) {
            /* If the setting is missing, consider child enabled. */
        }

        Category::SCPtr category;
        CannedQuery category_query(scope_id, query().query_string(), "");
        if (surfacing) {
            char title[500];
            snprintf(title, sizeof(title), _("%s Features"),
                     metadata.display_name().c_str());
            category = parent_reply->register_category(
                scope_id, title, "" /* icon */, category_query,
                CategoryRenderer(SURFACING_CATEGORY_DEFINITION));
        } else {
            char title[500];
            snprintf(title, sizeof(title), _("Results from %s"),
                     metadata.display_name().c_str());
            category = parent_reply->register_category(
                scope_id, title, "" /* icon */, category_query,
                CategoryRenderer(SEARCH_CATEGORY_DEFINITION));
        }
        auto subscope_reply = std::make_shared<SearchReceiver>(category, parent_reply, fwd);
        forwarders[i] = fwd = subscope_reply;
    }

    for (int i = 0; i < subscopes.size(); i++)
    {
        subsearch(subscopes[i].proxy(), query_string, department_id, filter_state, forwarders[i]);
    }
}
