/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by James Henstridge <james.henstridge@canonical.com>
 *
 */
#include <config.h>

#include <stdio.h>

#include <boost/regex.hpp>
#include <mediascanner/MediaFile.hh>
#include <unity/scopes/Category.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/ColumnLayout.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/VariantBuilder.h>

#include "video-scope.h"
#include "i18n.h"

#define MAX_RESULTS 100

using namespace mediascanner;
using namespace unity::scopes;

static const char AGGREGATOR_DEPT_ID[] = "aggregated:videoaggregator";

static const char LOCAL_CATEGORY_ICON[] = "/usr/share/icons/unity-icon-theme/places/svg/group-videos.svg";
static const char LOCAL_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "medium",
    "card-layout": "horizontal"
  },
  "components": {
    "title": "title",
    "art":  {
      "field": "art",
      "aspect-ratio": 1.5
    }
  }
}
)";

static const char AGGREGATOR_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "carousel",
    "overlay": true,
    "card-size": "medium"
  },
  "components": {
    "title": "title",
    "art":  {
      "field": "art",
      "aspect-ratio": 1.5
    }
  }
}
)";

// Category renderer to use when presenting search results
// FIXME: This should use list category-layout (LP #1279279)
static const char SEARCH_CATEGORY_DEFINITION[] = R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "small"
  },
  "components": {
    "title": "title",
    "art":  "art"
  }
}
)";

enum class VideoType {
    ALL,
    CAMERA,
    DOWNLOADS,
};

void VideoScope::start(std::string const&) {
    setlocale(LC_ALL, "");
    store.reset(new MediaStore(MS_READ_ONLY));
}

void VideoScope::stop() {
    store.reset();
}

SearchQueryBase::UPtr VideoScope::search(CannedQuery const &q,
                                         SearchMetadata const& hints) {
    SearchQueryBase::UPtr query(new VideoQuery(*this, q, hints));
    return query;
}

PreviewQueryBase::UPtr VideoScope::preview(Result const& result,
                                    ActionMetadata const& hints) {
    PreviewQueryBase::UPtr previewer(new VideoPreview(*this, result, hints));
    return previewer;
}

VideoQuery::VideoQuery(VideoScope &scope, CannedQuery const& query, SearchMetadata const& hints)
    : SearchQueryBase(query, hints),
      scope(scope) {
}

void VideoQuery::cancelled() {
}

static bool from_camera(const std::string &filename) {
    static const boost::regex pattern(R"(.*/video\d{8}_\d{4}\.mp4$)");
    return boost::regex_match(filename, pattern);
}

void VideoQuery::run(SearchReplyProxy const&reply) {
    bool surfacing = query().query_string() == "";
    bool is_aggregated = query().department_id() == AGGREGATOR_DEPT_ID;

    Department::SPtr root_dept = Department::create("", query(), _("Everything"));
    root_dept->set_subdepartments({
            Department::create("camera", query(), _("My Roll")),
            Department::create("downloads", query(), _("Downloaded")),
        });
    if (is_aggregated) {
        root_dept->add_subdepartment(
            Department::create(AGGREGATOR_DEPT_ID, query(), "dummy"));
    }
    reply->register_departments(root_dept);

    VideoType department = VideoType::ALL;
    if (query().department_id() == "camera") {
        department = VideoType::CAMERA;
    } else if (query().department_id() == "downloads") {
        department = VideoType::DOWNLOADS;
    }

    Category::SCPtr cat;
    if (is_aggregated) {
        cat = reply->register_category(
            "local", _("My Videos"), LOCAL_CATEGORY_ICON,
            CannedQuery(query().scope_id(), query().query_string(), ""),
            CategoryRenderer(surfacing ? AGGREGATOR_CATEGORY_DEFINITION : SEARCH_CATEGORY_DEFINITION));
    } else {
        cat = reply->register_category(
            "local", _("My Videos"), LOCAL_CATEGORY_ICON,
            CategoryRenderer(surfacing ? LOCAL_CATEGORY_DEFINITION : SEARCH_CATEGORY_DEFINITION));
    }
    for (const auto &media : scope.store->query(query().query_string(), VideoMedia, MAX_RESULTS)) {
        // Filter results if we are in a department
        switch (department) {
        case VideoType::ALL:
            break;
        case VideoType::CAMERA:
            if (!from_camera(media.getFileName())) {
                continue;
            }
            break;
        case VideoType::DOWNLOADS:
            if (from_camera(media.getFileName())) {
                continue;
            }
            break;
        }

        std::string uri = media.getUri();
        std::string imguri;
        if (uri.find("file://") == 0) {
            imguri = "thumbnailer://" + uri.substr(7);
            uri = "video://" + uri.substr(7); // replace file:// with video://
        }

        CategorisedResult res(cat);
        res.set_uri(uri);
        res.set_dnd_uri(uri);
        res.set_art(imguri.size() == 0 ? media.getUri() : imguri);
        res.set_title(media.getTitle());

        res["duration"] =media.getDuration();
        // res["width"] = media.getWidth();
        // res["height"] = media.getHeight();

        if(!reply->push(res))
        {
            return;
        }
    }
}

VideoPreview::VideoPreview(VideoScope &scope, Result const& result, ActionMetadata const& hints)
    : PreviewQueryBase(result, hints),
      scope(scope) {
}

void VideoPreview::cancelled() {
}

void VideoPreview::run(PreviewReplyProxy const& reply)
{
    ColumnLayout layout1col(1), layout2col(2), layout3col(3);
    layout1col.add_column({"video", "header", "actions"});

    layout2col.add_column({"video", "header"});
    layout2col.add_column({"actions"});

    layout3col.add_column({"video", "header"});
    layout3col.add_column({"actions"});
    layout3col.add_column({});
    reply->register_layout({layout1col, layout2col, layout3col});

    PreviewWidget header("header", "header");
    header.add_attribute_mapping("title", "title");

    PreviewWidget video("video", "video");
    video.add_attribute_mapping("source", "uri");

    PreviewWidget actions("actions", "actions");
    {
        VariantBuilder builder;
        builder.add_tuple({
                {"id", Variant("play")},
                {"label", Variant(_("Play"))}
            });
        actions.add_attribute_value("actions", builder.end());
    }

    reply->push({video, header, actions});
}

extern "C" ScopeBase * UNITY_SCOPE_CREATE_FUNCTION() {
    return new VideoScope;
}

extern "C" void UNITY_SCOPE_DESTROY_FUNCTION(ScopeBase *scope) {
    delete scope;
}
