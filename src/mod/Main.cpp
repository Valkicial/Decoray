// AI Deco: editor integration (toolbar button, hover button, keybind).
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/loader/SettingV3.hpp>

#include "Bridge.hpp"
#include "DecoPopup.hpp"

using namespace geode::prelude;
using namespace aideco;

namespace {

CCSprite* makeIcon(float scale) {
    auto spr = ButtonSprite::create("AI", 22, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.55f);
    spr->setScale(scale);
    return spr;
}

} // namespace

class $modify(DecoEditorUI, EditorUI) {
    struct Fields {
        CCMenu* hoverMenu = nullptr;
        CCMenuItemSpriteExtra* hoverBtn = nullptr;
        CCDrawNode* highlight = nullptr;
        Ref<GameObject> hovered;
        float grace = 0.f;
    };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;
        DecoPopup::resetSession();

        // toolbar button (uses the selection, or the screen center)
        auto btn = CCMenuItemSpriteExtra::create(makeIcon(0.8f), this, menu_selector(DecoEditorUI::onDecoToolbar));
        btn->setID("ai-deco-button"_spr);
        if (auto menu = this->getChildByID("editor-buttons-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        else {
            auto own = CCMenu::create();
            own->setID("ai-deco-menu"_spr);
            auto win = CCDirector::get()->getWinSize();
            own->setPosition({ win.width - 92.f, win.height - 22.f });
            own->addChild(btn);
            this->addChild(own, 10);
        }

        // floating button that follows the hovered block
        auto f = m_fields.self();
        f->hoverMenu = CCMenu::create();
        f->hoverMenu->setID("ai-deco-hover-menu"_spr);
        f->hoverMenu->setPosition({ 0, 0 });
        f->hoverBtn = CCMenuItemSpriteExtra::create(makeIcon(0.55f), this, menu_selector(DecoEditorUI::onDecoHover));
        f->hoverBtn->setVisible(false);
        f->hoverMenu->addChild(f->hoverBtn);
        this->addChild(f->hoverMenu, 100);

        f->highlight = CCDrawNode::create();
        f->highlight->setID("ai-deco-highlight"_spr);
        if (editorLayer->m_objectLayer) editorLayer->m_objectLayer->addChild(f->highlight, 9999);

        this->schedule(schedule_selector(DecoEditorUI::updateDecoHover), 0.05f);
        return true;
    }

    void hideHover() {
        auto f = m_fields.self();
        if (f->hoverBtn) f->hoverBtn->setVisible(false);
        if (f->highlight) f->highlight->clear();
        f->hovered = nullptr;
    }

    void updateDecoHover(float dt) {
        auto f = m_fields.self();
        if (!f->hoverBtn || !m_editorLayer) return;
        if (!Mod::get()->getSettingValue<bool>("show-hover-button") || DecoPopup::isOpen() ||
            m_editorLayer->m_playbackMode != PlaybackMode::Not || !this->isVisible()) {
            this->hideHover();
            return;
        }
#if defined(GEODE_IS_DESKTOP)
        auto mouse = getMousePos();
        // keep the button while the cursor is on it
        if (f->hoverBtn->isVisible()) {
            auto local = f->hoverMenu->convertToNodeSpace(mouse);
            if (f->hoverBtn->boundingBox().containsPoint(local)) return;
        }
        auto world = m_editorLayer->m_objectLayer->convertToNodeSpace(mouse);
        GameObject* obj = m_editorLayer->objectAtPosition(world);
        if (obj) {
            f->grace = 0.6f;
            if (obj != static_cast<GameObject*>(f->hovered)) {
                f->hovered = obj;
                auto r = objectRect(obj);
                f->highlight->clear();
                CCPoint verts[4] = {
                    r.origin, ccp(r.getMaxX(), r.getMinY()), ccp(r.getMaxX(), r.getMaxY()), ccp(r.getMinX(), r.getMaxY())
                };
                f->highlight->drawPolygon(verts, 4, ccc4f(0.3f, 0.85f, 1.f, 0.08f), 1.2f, ccc4f(0.3f, 0.85f, 1.f, 0.9f));
            }
        }
        if (f->hovered && !f->hovered->getParent()) this->hideHover();
        if (f->hovered) {
            // keep the button on the top-right corner of the block, in UI space (camera may move)
            auto r = objectRect(f->hovered);
            auto corner = m_editorLayer->m_objectLayer->convertToWorldSpace(ccp(r.getMaxX(), r.getMaxY()));
            f->hoverBtn->setPosition(f->hoverMenu->convertToNodeSpace(corner) + ccp(8, 8));
            f->hoverBtn->setVisible(true);
        }
        if (!obj) {
            f->grace -= dt;
            if (f->grace <= 0) this->hideHover();
        }
#else
        this->hideHover();
#endif
    }

    // Builds the target from a hovered object, the selection, or a position.
    DecoTarget buildTarget(GameObject* hovered, CCPoint world, bool preferSelection) {
        DecoTarget t;
        t.target.anchor = { world.x, world.y };
        size_t maxBlocks = size_t(std::clamp<int64_t>(Mod::get()->getSettingValue<int64_t>("max-structure-blocks"), 1, 2000));
        auto sel = this->getSelectedObjects();
        bool hasSel = sel && sel->count() > 0;

        if (hovered && hasSel && sel->count() > 1 && sel->containsObject(hovered)) {
            t.target.structure = true;
            t.target.blocks = rectsOf(sel);
            t.label = fmt::format("Decorating your selection ({} objects)", sel->count());
        }
        else if (hovered) {
            t.target.structure = true;
            t.target.blocks = gatherStructure(m_editorLayer, hovered, maxBlocks);
            t.label = t.target.blocks.size() > 1
                ? fmt::format("Decorating the hovered structure ({} blocks)", t.target.blocks.size())
                : std::string("Decorating the hovered object");
        }
        else if (preferSelection && hasSel) {
            t.target.structure = true;
            t.target.blocks = rectsOf(sel);
            t.label = fmt::format("Decorating your selection ({} objects)", sel->count());
        }
        else {
            t.target.structure = false;
            t.label = fmt::format("New design at ({:.0f}, {:.0f})", world.x, world.y);
        }
        return t;
    }

    void openDeco(DecoTarget target) {
        if (DecoPopup::isOpen()) return;
        this->hideHover();
        if (auto popup = DecoPopup::create(this, std::move(target))) popup->show();
    }

    void onDecoHover(CCObject*) {
        auto f = m_fields.self();
        Ref<GameObject> obj = f->hovered;
        CCPoint world = obj ? obj->getPosition() : CCPoint{ 0, 0 };
        this->openDeco(this->buildTarget(obj, world, false));
    }

    void onDecoToolbar(CCObject*) {
        auto win = CCDirector::get()->getWinSize();
        auto world = m_editorLayer->m_objectLayer->convertToNodeSpace(ccp(win.width / 2, win.height / 2));
        this->openDeco(this->buildTarget(nullptr, world, true));
    }

    // keybind: decorate whatever is under the cursor (or design at the cursor)
    void onDecoKey() {
        if (!m_editorLayer || m_editorLayer->m_playbackMode != PlaybackMode::Not) return;
#if defined(GEODE_IS_DESKTOP)
        auto mouse = getMousePos();
#else
        auto winSize = CCDirector::get()->getWinSize();
        auto mouse = ccp(winSize.width / 2, winSize.height / 2);
#endif
        auto world = m_editorLayer->m_objectLayer->convertToNodeSpace(mouse);
        GameObject* obj = m_editorLayer->objectAtPosition(world);
        this->openDeco(this->buildTarget(obj, world, false));
    }
};

$on_mod(Loaded) {
    listenForKeybindSettingPresses("open-key", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat || DecoPopup::isOpen()) return false;
        auto lel = LevelEditorLayer::get();
        if (!lel || !lel->m_editorUI) return false;
        static_cast<DecoEditorUI*>(lel->m_editorUI)->onDecoKey();
        return true;
    });
}
