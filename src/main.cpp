#include <Geode/modify/MenuLayer.hpp>
#include <Geode/Geode.hpp>

#include "SaveToolPopup.hpp"

using namespace geode::prelude;

class $modify(SaveToolMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // This tool only makes sense on Android (it edits GD's on-device
        // save .dat files at their public/media path), so the button is
        // only added on that platform.
#if defined(GEODE_IS_ANDROID)
        if (auto menu = typeinfo_cast<CCMenu*>(this->getChildByID("bottom-menu"))) {
            auto sprite = CCSprite::create("button.png"_spr);
            if (sprite) {
                sprite->setScale(0.9f);
            }

            auto btn = CCMenuItemSpriteExtra::create(
                sprite ? static_cast<CCNode*>(sprite)
                       : static_cast<CCNode*>(ButtonSprite::create("Save", "bigFont.fnt", "GJ_button_01.png", 0.6f)),
                this,
                menu_selector(SaveToolMenuLayer::onOpenSaveTool)
            );
            btn->setID("gd-save-tool-button"_spr);

            menu->addChild(btn);
            menu->updateLayout();
        }
#endif

        return true;
    }

    void onOpenSaveTool(CCObject*) {
        SaveToolPopup::create()->show();
    }
};
