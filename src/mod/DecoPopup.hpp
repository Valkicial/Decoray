#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/SliderNode.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

#include "../core/DecoCore.hpp"

namespace aideco {

struct DecoTarget {
    deco::Target target;
    std::string label;
};

class DecoPopup : public geode::Popup {
public:
    static DecoPopup* create(EditorUI* ui, DecoTarget target);
    static bool isOpen();
    // forget the last design (called when a new editor opens)
    static void resetSession();

protected:
    bool init(EditorUI* ui, DecoTarget target);
    void onClose(cocos2d::CCObject*) override;
    ~DecoPopup() override;

    void onGenerate(cocos2d::CCObject*);
    void onReroll(cocos2d::CCObject*);
    void onPreset(cocos2d::CCObject*);
    void onToggle(cocos2d::CCObject*);
    void onSaveExample(cocos2d::CCObject*);
    void onSettings(cocos2d::CCObject*);
    void onOpenExamples(cocos2d::CCObject*);

    void generate(bool replaceLast);
    void finish(deco::Design const& design, bool replaceLast);
    void setStatus(std::string const& text, cocos2d::ccColor3B color = { 200, 200, 210 });
    void setBusy(bool busy);
    void addToggle(char const* label, int tag, bool on, cocos2d::CCPoint pos);
    void saveOptions();

    EditorUI* m_ui = nullptr;
    DecoTarget m_target;
    geode::TextInput* m_input = nullptr;
    geode::SliderNode* m_limitSlider = nullptr;
    geode::SliderNode* m_sizeSlider = nullptr;
    cocos2d::CCLabelBMFont* m_limitLabel = nullptr;
    cocos2d::CCLabelBMFont* m_sizeLabel = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    CCMenuItemSpriteExtra* m_generateBtn = nullptr;
    CCMenuItemSpriteExtra* m_rerollBtn = nullptr;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_request;

    int m_limit = 80;
    float m_size = 4.f;
    bool m_useClaude = false;
    bool m_pixel = false;
    bool m_autoColors = true;
    bool m_useExamples = true;
    bool m_busy = false;
};

} // namespace aideco
