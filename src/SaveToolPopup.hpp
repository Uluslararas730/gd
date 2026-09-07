#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

class SaveToolPopup : public geode::Popup<> {
protected:
    bool setup() override;

    void onDecrypt(cocos2d::CCObject* sender);
    void onEncrypt(cocos2d::CCObject* sender);
    void onTogglePrettify(cocos2d::CCObject* sender);

    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    bool m_prettify[4] = { false, false, false, false };

public:
    static SaveToolPopup* create();
};
