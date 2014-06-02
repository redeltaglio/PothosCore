// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "GraphEditor/GraphEditor.hpp"
#include "GraphEditor/GraphDraw.hpp"
#include "GraphObjects/GraphBlock.hpp"
#include "GraphObjects/GraphBreaker.hpp"
#include "GraphObjects/GraphConnection.hpp"
#include <Pothos/Exception.hpp>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <QScrollArea>
#include <cassert>

/***********************************************************************
 * Per-type creation routine
 **********************************************************************/
static void loadPages(GraphEditor *editor, Poco::JSON::Array::Ptr pages, const std::string &type)
{
    for (size_t pageNo = 0; pageNo < pages->size(); pageNo++)
    {
        auto pageObj = pages->getObject(pageNo);
        auto graphObjects = pageObj->getArray("graphObjects");
        auto parent = dynamic_cast<QScrollArea *>(editor->widget(pageNo))->widget();

        for (size_t objIndex = 0; objIndex < graphObjects->size(); objIndex++)
        {
            GraphObject *obj = nullptr;
            std::string errorMsg;
            try
            {
                const auto jGraphObj = graphObjects->getObject(objIndex);
                if (not jGraphObj) continue;
                const auto what = jGraphObj->getValue<std::string>("what");
                if (what != type) continue;
                if (type == "Block") obj = new GraphBlock(parent);
                if (type == "Breaker") obj = new GraphBreaker(parent);
                if (type == "Connection") obj = new GraphConnection(parent);
                if (obj != nullptr) obj->deserialize(jGraphObj);
            }
            catch (const Poco::Exception &ex)
            {
                errorMsg = ex.displayText();
            }
            catch (const Pothos::Exception &ex)
            {
                errorMsg = ex.displayText();
            }
            catch (const std::exception &ex)
            {
                errorMsg = ex.what();
            }
            catch (...)
            {
                errorMsg = "unknown";
            }
            if (not errorMsg.empty())
            {
                poco_error(Poco::Logger::get("PothosGui.GraphEditor.loadState"), errorMsg);
                delete obj;
            }
        }
    }
}

/***********************************************************************
 * Deserialization routine
 **********************************************************************/
void GraphEditor::loadState(std::istream &is)
{
    Poco::JSON::Parser p; p.parse(is);
    auto pages = p.getHandler()->asVar().extract<Poco::JSON::Array::Ptr>();
    assert(pages);

    ////////////////////////////////////////////////////////////////////
    // clear existing stuff
    ////////////////////////////////////////////////////////////////////
    for (int pageNo = 0; pageNo < this->count(); pageNo++)
    {
        for (auto graphObj : this->getGraphDraw(pageNo)->getGraphObjects())
        {
            delete graphObj;
        }

        //delete page later so we dont mess up the tabs
        this->widget(pageNo)->deleteLater();
    }
    this->clear(); //removes all tabs from this widget

    ////////////////////////////////////////////////////////////////////
    // create pages
    ////////////////////////////////////////////////////////////////////
    for (size_t pageNo = 0; pageNo < pages->size(); pageNo++)
    {
        auto pageObj = pages->getObject(pageNo);
        auto pageName = pageObj->getValue<std::string>("pageName");
        auto graphObjects = pageObj->getArray("graphObjects");
        auto page = makeGraphPage(this);
        assert(dynamic_cast<QScrollArea *>(page) != nullptr);
        this->insertTab(int(pageNo), page, QString::fromStdString(pageName));
        if (pageObj->getValue<bool>("selected")) this->setCurrentIndex(pageNo);
    }

    ////////////////////////////////////////////////////////////////////
    // create graph objects
    ////////////////////////////////////////////////////////////////////
    loadPages(this, pages, "Block");
    loadPages(this, pages, "Breaker");
    loadPages(this, pages, "Connection");
}