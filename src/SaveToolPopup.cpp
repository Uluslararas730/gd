#include "SaveToolPopup.hpp"
#include "SaveTool.hpp"

using namespace geode::prelude;

namespace {

constexpr const char* kSectionTitles[4] = {
    "CCGameManager (Save 1)",
    "CCGameManager2 (Save 2)",
    "CCLocalLevels (Save 3)",
    "CCLocalLevels2 (Save 4)"
};

constexpr const char* kBaseNames[4] = {
    "CCGameManager",
    "CCGameManager2",
    "CCLocalLevels",
    "CCLocalLevels2"
};

}

SaveToolPopup* SaveToolPopup::create() {
    auto ret = new SaveToolPopup();
    if (ret && ret->initAnchored(380.f, 260.f)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SaveToolPopup::setup() {
    this->setTitle("GD Save Tool");

    auto winSize = m_mainLayer->getContentSize();

    m_statusLabel = CCLabelBMFont::create("Islem yapmak icin bir buton secin.", "chatFont.fnt");
    m_statusLabel->setScale(0.45f);
    m_statusLabel->setPosition(winSize.width / 2.f, winSize.height - 34.f);
    m_mainLayer->addChild(m_statusLabel);

    const float top = winSize.height - 58.f;
    const float rowHeight = 46.f;

    for (int i = 0; i < 4; i++) {
        float rowY = top - i * rowHeight;

        auto titleLabel = CCLabelBMFont::create(kSectionTitles[i], "goldFont.fnt");
        titleLabel->setScale(0.35f);
        titleLabel->setAnchorPoint({ 0.f, 0.5f });
        titleLabel->setPosition(14.f, rowY);
        m_mainLayer->addChild(titleLabel);

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(menu);

        auto decSprite = ButtonSprite::create("Decrypt", "bigFont.fnt", "GJ_button_01.png", 0.8f);
        auto decBtn = CCMenuItemSpriteExtra::create(
            decSprite, this, menu_selector(SaveToolPopup::onDecrypt)
        );
        decBtn->setTag(i);
        decBtn->setPosition(80.f, rowY - 20.f);
        menu->addChild(decBtn);

        auto encSprite = ButtonSprite::create("Encrypt", "bigFont.fnt", "GJ_button_02.png", 0.8f);
        auto encBtn = CCMenuItemSpriteExtra::create(
            encSprite, this, menu_selector(SaveToolPopup::onEncrypt)
        );
        encBtn->setTag(i);
        encBtn->setPosition(180.f, rowY - 20.f);
        menu->addChild(encBtn);

        auto pretSprite = ButtonSprite::create("Format: OFF", "bigFont.fnt", "GJ_button_04.png", 0.7f);
        auto pretBtn = CCMenuItemSpriteExtra::create(
            pretSprite, this, menu_selector(SaveToolPopup::onTogglePrettify)
        );
        pretBtn->setTag(i);
        pretBtn->setPosition(300.f, rowY - 20.f);
        menu->addChild(pretBtn);
    }

    return true;
}

void SaveToolPopup::onDecrypt(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    auto [success, msg] = SaveTool::decrypt(kBaseNames[idx], m_prettify[idx]);
    m_statusLabel->setString(msg.c_str());
    m_statusLabel->setColor(success ? ccColor3B{ 80, 220, 100 } : ccColor3B{ 255, 80, 80 });
}

void SaveToolPopup::onEncrypt(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    auto [success, msg] = SaveTool::encrypt(kBaseNames[idx]);
    m_statusLabel->setString(msg.c_str());
    m_statusLabel->setColor(success ? ccColor3B{ 80, 220, 100 } : ccColor3B{ 255, 80, 80 });
}

void SaveToolPopup::onTogglePrettify(CCObject* sender) {
    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    int idx = item->getTag();
    m_prettify[idx] = !m_prettify[idx];

    auto newSprite = ButtonSprite::create(
        m_prettify[idx] ? "Format: ON" : "Format: OFF",
        "bigFont.fnt",
        m_prettify[idx] ? "GJ_button_01.png" : "GJ_button_04.png",
        0.7f
    );
    item->setNormalImage(newSprite);
    item->setContentSize(newSprite->getContentSize());
}
