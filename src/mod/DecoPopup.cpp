#include "DecoPopup.hpp"
#include "Bridge.hpp"
#include "Claude.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/binding/EditorUI.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/Notification.hpp>

#include <chrono>

using namespace geode::prelude;

namespace aideco {

namespace {

// state that survives closing/reopening the popup
struct Session {
    Ref<CCArray> lastObjects;
    std::optional<DecoTarget> lastTarget;
    std::string lastDescription;
    uint32_t seed = 1;
    bool open = false;
};
Session& session() {
    static Session s;
    return s;
}

enum Tag { TAG_CLAUDE = 1, TAG_PIXEL, TAG_COLORS, TAG_EXAMPLES };

char const* PRESETS[] = { "neon", "nature", "lava", "ice", "space", "castle", "candy", "spooky", "tech" };

uint32_t freshSeed() {
    auto t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return uint32_t(t ^ (t >> 32)) | 1u;
}

} // namespace

bool DecoPopup::isOpen() { return session().open; }

void DecoPopup::resetSession() {
    session().lastObjects = nullptr;
    session().lastTarget.reset();
    session().open = false;
}

DecoPopup* DecoPopup::create(EditorUI* ui, DecoTarget target) {
    auto ret = new DecoPopup();
    if (ret->init(ui, std::move(target))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

DecoPopup::~DecoPopup() {
    session().open = false;
}

bool DecoPopup::init(EditorUI* ui, DecoTarget target) {
    if (!Popup::init(420.f, 290.f)) return false;
    m_ui = ui;
    m_target = std::move(target);
    session().open = true;

    auto mod = Mod::get();
    m_limit = std::clamp(int(mod->getSavedValue<int64_t>("limit", 80)), 5, 1000);
    m_size = std::clamp(float(mod->getSavedValue<double>("size", 4.0)), 1.f, 20.f);
    m_useClaude = mod->getSavedValue<bool>("use-claude", false);
    m_pixel = mod->getSavedValue<bool>("pixel", false);
    m_autoColors = mod->getSettingValue<bool>("auto-colors");
    m_useExamples = mod->getSavedValue<bool>("use-examples", true);
    Library::get().load();

    this->setTitle("AI Deco Generator");

    auto targetLabel = CCLabelBMFont::create(m_target.label.c_str(), "chatFont.fnt");
    targetLabel->setScale(0.6f);
    targetLabel->setColor({ 150, 220, 255 });
    m_mainLayer->addChildAtPosition(targetLabel, Anchor::Top, ccp(0, -40));

    m_input = TextInput::create(380.f, "Describe it: e.g. mossy castle wall with glowing windows", "chatFont.fnt");
    m_input->setCommonFilter(CommonFilter::Any);
    m_input->setMaxCharCount(280);
    m_input->setString(session().lastDescription);
    m_mainLayer->addChildAtPosition(m_input, Anchor::Top, ccp(0, -66));

    // style presets: tap to add a word to the description
    float px = -((std::size(PRESETS) - 1) * 44.f) / 2;
    int i = 0;
    for (auto preset : PRESETS) {
        auto spr = ButtonSprite::create(preset, "bigFont.fnt", "GJ_button_05.png", 0.6f);
        spr->setScale(0.42f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(DecoPopup::onPreset));
        btn->setUserObject(CCString::create(preset));
        m_buttonMenu->addChildAtPosition(btn, Anchor::Top, ccp(px + i * 44.f, -96));
        ++i;
    }

    // object limit slider
    m_limitLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_limitLabel->setScale(0.55f);
    m_mainLayer->addChildAtPosition(m_limitLabel, Anchor::Center, ccp(-100, 26));
    m_limitSlider = SliderNode::create([this](SliderNode*, float v) {
        m_limit = std::clamp(int(std::round(v / 5.f) * 5), 5, 1000);
        m_limitLabel->setString(fmt::format("Object limit: {}", m_limit).c_str());
    });
    m_limitSlider->setContentSize({ 160.f, 14.f });
    m_limitSlider->setAnchorPoint({ 0.5f, 0.5f });
    m_limitSlider->setMin(5.f);
    m_limitSlider->setMax(1000.f);
    m_mainLayer->addChildAtPosition(m_limitSlider, Anchor::Center, ccp(-100, 6));
    m_limitSlider->setValue(float(m_limit));
    m_limitLabel->setString(fmt::format("Object limit: {}", m_limit).c_str());

    // size slider
    m_sizeLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_sizeLabel->setScale(0.55f);
    m_mainLayer->addChildAtPosition(m_sizeLabel, Anchor::Center, ccp(100, 26));
    m_sizeSlider = SliderNode::create([this](SliderNode*, float v) {
        m_size = std::clamp(std::round(v * 2.f) / 2.f, 1.f, 20.f);
        m_sizeLabel->setString(fmt::format("Size: {:.1f} blocks", m_size).c_str());
    });
    m_sizeSlider->setContentSize({ 160.f, 14.f });
    m_sizeSlider->setAnchorPoint({ 0.5f, 0.5f });
    m_sizeSlider->setMin(1.f);
    m_sizeSlider->setMax(20.f);
    m_mainLayer->addChildAtPosition(m_sizeSlider, Anchor::Center, ccp(100, 6));
    m_sizeSlider->setValue(m_size);
    m_sizeLabel->setString(fmt::format("Size: {:.1f} blocks", m_size).c_str());

    // toggles
    this->addToggle(claudeConfigured() ? "Claude AI" : "Claude AI (no key)", TAG_CLAUDE, m_useClaude, ccp(-180, -30));
    this->addToggle("Pixel art", TAG_PIXEL, m_pixel, ccp(-80, -30));
    this->addToggle("Auto colors", TAG_COLORS, m_autoColors, ccp(10, -30));
    this->addToggle(fmt::format("My examples ({})", Library::get().size()).c_str(), TAG_EXAMPLES, m_useExamples, ccp(110, -30));

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.55f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Bottom, ccp(0, 62));
    this->setStatus(m_target.target.structure
        ? "Your blocks get covered by the design. Tip: name a style, colors and details."
        : "Free design at the cursor. Tip: name a subject, a style and colors.");

    // bottom buttons
    auto genSpr = ButtonSprite::create("Generate", "goldFont.fnt", "GJ_button_01.png", 0.85f);
    m_generateBtn = CCMenuItemSpriteExtra::create(genSpr, this, menu_selector(DecoPopup::onGenerate));
    m_buttonMenu->addChildAtPosition(m_generateBtn, Anchor::Bottom, ccp(0, 30));

    auto rerollSpr = ButtonSprite::create("Reroll last", "goldFont.fnt", "GJ_button_02.png", 0.6f);
    rerollSpr->setScale(0.8f);
    m_rerollBtn = CCMenuItemSpriteExtra::create(rerollSpr, this, menu_selector(DecoPopup::onReroll));
    m_buttonMenu->addChildAtPosition(m_rerollBtn, Anchor::Bottom, ccp(-132, 30));
    m_rerollBtn->setEnabled(session().lastObjects != nullptr);
    rerollSpr->setOpacity(session().lastObjects ? 255 : 120);

    auto saveSpr = ButtonSprite::create("Save example", "goldFont.fnt", "GJ_button_04.png", 0.6f);
    saveSpr->setScale(0.8f);
    auto saveBtn = CCMenuItemSpriteExtra::create(saveSpr, this, menu_selector(DecoPopup::onSaveExample));
    m_buttonMenu->addChildAtPosition(saveBtn, Anchor::Bottom, ccp(135, 30));

    auto gearSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    gearSpr->setScale(0.55f);
    auto gearBtn = CCMenuItemSpriteExtra::create(gearSpr, this, menu_selector(DecoPopup::onSettings));
    m_buttonMenu->addChildAtPosition(gearBtn, Anchor::TopRight, ccp(-22, -22));

    auto folderSpr = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
    folderSpr->setScale(0.55f);
    auto folderBtn = CCMenuItemSpriteExtra::create(folderSpr, this, menu_selector(DecoPopup::onOpenExamples));
    m_buttonMenu->addChildAtPosition(folderBtn, Anchor::BottomRight, ccp(-20, 20));

    return true;
}

void DecoPopup::addToggle(char const* label, int tag, bool on, CCPoint pos) {
    auto toggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(DecoPopup::onToggle), 0.55f);
    toggle->setTag(tag);
    toggle->toggle(on);
    m_buttonMenu->addChildAtPosition(toggle, Anchor::Center, pos);
    auto text = CCLabelBMFont::create(label, "bigFont.fnt");
    text->setScale(0.3f);
    text->setAnchorPoint({ 0.f, 0.5f });
    m_mainLayer->addChildAtPosition(text, Anchor::Center, pos + ccp(13, 0));
}

void DecoPopup::onToggle(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    bool now = !toggle->isToggled(); // callback fires before the state flips
    switch (toggle->getTag()) {
        case TAG_CLAUDE:
            m_useClaude = now;
            if (now && !claudeConfigured())
                this->setStatus("Add your Claude API key in the mod settings (gear icon) to use Claude.", { 255, 200, 90 });
            break;
        case TAG_PIXEL: m_pixel = now; break;
        case TAG_COLORS: m_autoColors = now; break;
        case TAG_EXAMPLES: m_useExamples = now; break;
    }
    this->saveOptions();
}

void DecoPopup::saveOptions() {
    auto mod = Mod::get();
    mod->setSavedValue<int64_t>("limit", m_limit);
    mod->setSavedValue<double>("size", m_size);
    mod->setSavedValue<bool>("use-claude", m_useClaude);
    mod->setSavedValue<bool>("pixel", m_pixel);
    mod->setSavedValue<bool>("use-examples", m_useExamples);
    mod->setSettingValue<bool>("auto-colors", m_autoColors);
}

void DecoPopup::onPreset(CCObject* sender) {
    auto word = static_cast<CCString*>(static_cast<CCNode*>(sender)->getUserObject())->getCString();
    std::string cur = m_input->getString();
    if (!cur.empty() && cur.back() != ' ') cur += " ";
    cur += word;
    m_input->setString(cur);
}

void DecoPopup::onSettings(CCObject*) {
    openSettingsPopup(Mod::get(), false);
}

void DecoPopup::onOpenExamples(CCObject*) {
    std::error_code ec;
    std::filesystem::create_directories(Library::get().folder(), ec);
    utils::file::openFolder(Library::get().folder());
}

void DecoPopup::onSaveExample(CCObject*) {
    std::string name = m_input->getString();
    auto res = Library::get().addFromSelection(m_ui, name);
    if (!res) {
        this->setStatus(res.unwrapErr(), { 255, 120, 120 });
        return;
    }
    this->setStatus(fmt::format("Saved \"{}\" ({} examples). Describe it with similar words to reuse it.",
        name.empty() ? "example" : name, Library::get().size()), { 140, 255, 140 });
}

void DecoPopup::setStatus(std::string const& text, ccColor3B color) {
    std::string t = text.size() > 110 ? text.substr(0, 107) + "..." : text;
    m_status->setString(t.c_str());
    m_status->setColor(color);
    m_status->limitLabelWidth(390.f, 0.55f, 0.3f);
}

void DecoPopup::setBusy(bool busy) {
    m_busy = busy;
    m_generateBtn->setEnabled(!busy);
    m_rerollBtn->setEnabled(!busy && session().lastObjects);
    static_cast<ButtonSprite*>(m_generateBtn->getNormalImage())->setString(busy ? "Thinking..." : "Generate");
}

void DecoPopup::onGenerate(CCObject*) { this->generate(false); }
void DecoPopup::onReroll(CCObject*) { this->generate(true); }

void DecoPopup::generate(bool replaceLast) {
    if (m_busy) return;
    std::string desc = m_input->getString();
    if (replaceLast && desc.empty()) desc = session().lastDescription;
    if (desc.empty()) {
        this->setStatus("Type a description first (or tap a style button).", { 255, 200, 90 });
        return;
    }
    this->saveOptions();
    session().lastDescription = desc;
    session().seed = freshSeed();

    if (replaceLast && session().lastTarget) m_target = *session().lastTarget;

    // a saved example that matches the words wins over the procedural engine
    if (m_useExamples && !m_useClaude) {
        auto matches = Library::get().match(desc, 1);
        if (!matches.empty()) {
            if (replaceLast && session().lastObjects) deleteObjects(m_ui, session().lastObjects);
            CCPoint at = m_target.target.structure && !m_target.target.blocks.empty()
                ? ccp(m_target.target.blocks.front().cx(), m_target.target.blocks.front().cy())
                : ccp(m_target.target.anchor.x, m_target.target.anchor.y);
            auto res = Library::get().place(m_ui, *matches.front(), at);
            if (!res) { this->setStatus(res.unwrapErr(), { 255, 120, 120 }); return; }
            session().lastObjects = res.unwrap().objects;
            session().lastTarget = m_target;
            Notification::create(fmt::format("Placed your example \"{}\"", matches.front()->name), NotificationIcon::Success)->show();
            this->onClose(nullptr);
            return;
        }
    }

    deco::Options opts;
    opts.limit = m_limit;
    opts.seed = session().seed;
    opts.pixel = m_pixel;
    opts.sizeBlocks = m_size;
    auto design = deco::generate(desc, m_target.target, opts);

    if (!m_useClaude) {
        this->finish(design, replaceLast);
        return;
    }
    if (!claudeConfigured()) {
        this->setStatus("No Claude API key set, used the built-in generator instead.", { 255, 200, 90 });
        this->finish(design, replaceLast);
        return;
    }

    AIRequest req;
    req.description = desc;
    req.structure = m_target.target.structure;
    req.blocks = m_target.target.blocks;
    req.anchor = m_target.target.anchor;
    req.sizeBlocks = m_size;
    req.limit = m_limit;
    req.draft = design;
    if (m_useExamples) {
        for (auto ex : Library::get().match(desc, 2)) req.examples += Library::describe(*ex, 120) + "\n";
    }

    auto endpoint = Mod::get()->getSettingValue<std::string>("claude-endpoint");
    if (endpoint.empty()) endpoint = "https://api.anthropic.com/v1/messages";

    this->setBusy(true);
    this->setStatus("Claude is designing... (10-60 s)", { 150, 220, 255 });
    m_request.spawn(
        "AI Deco Claude request",
        buildClaudeRequest(req).post(endpoint),
        [this, req, replaceLast](web::WebResponse resp) {
            this->setBusy(false);
            auto parsed = parseClaudeResponse(resp, req);
            if (!parsed) {
                log::warn("Claude request failed: {}", parsed.unwrapErr());
                this->setStatus("Claude failed: " + parsed.unwrapErr(), { 255, 120, 120 });
                return;
            }
            this->finish(parsed.unwrap(), replaceLast);
        }
    );
}

void DecoPopup::finish(deco::Design const& design, bool replaceLast) {
    if (replaceLast && session().lastObjects) {
        deleteObjects(m_ui, session().lastObjects);
        session().lastObjects = nullptr;
    }
    PlaceSettings ps;
    ps.limit = m_limit;
    ps.pixel = m_pixel;
    ps.autoColors = m_autoColors;
    ps.noTouch = Mod::get()->getSettingValue<bool>("no-touch");
    ps.dontFade = Mod::get()->getSettingValue<bool>("dont-fade");
    ps.dontEnter = Mod::get()->getSettingValue<bool>("dont-enter");
    auto res = placeDesign(m_ui, design, ps);
    if (!res) {
        this->setStatus(res.unwrapErr(), { 255, 120, 120 });
        return;
    }
    auto placed = res.unwrap();
    session().lastObjects = placed.objects;
    session().lastTarget = m_target;

    log::info("AI Deco placed {} objects: {} ({})", placed.count, design.summary, placed.colorInfo);
    Notification::create(fmt::format("{} objects: {}", placed.count, design.summary), NotificationIcon::Success, 3.f)->show();
    if (Mod::get()->getSettingValue<bool>("close-after-generate")) this->onClose(nullptr);
    else {
        this->setStatus(fmt::format("Placed {} objects. {}", placed.count, placed.colorInfo), { 140, 255, 140 });
        m_rerollBtn->setEnabled(true);
        static_cast<ButtonSprite*>(m_rerollBtn->getNormalImage())->setOpacity(255);
    }
}

void DecoPopup::onClose(CCObject* sender) {
    m_request.cancel();
    session().open = false;
    Popup::onClose(sender);
}

} // namespace aideco
