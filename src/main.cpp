#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

namespace Buffer {
    constexpr int LEVELS[] = {0, 2, 4, 8};   // 0 = apagado
    inline int levelIdx = 0;
    inline int saved = 0;
    inline int pending[2] = {0, 0};
    inline bool held[2] = {false, false};
    inline int releaseIn[2] = {0, 0};

    inline int ticks() { return LEVELS[levelIdx]; }
    inline void clear() {
        pending[0] = pending[1] = 0;
        releaseIn[0] = releaseIn[1] = 0;
        held[0] = held[1] = false;
    }
}

static bool canBuffer(PlayerObject* p) {
    return p && !p->m_isShip && !p->m_isBird && !p->m_isDart && !p->m_isSwing;
}

class $modify(BufferGJBGL, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (button == 1) {
            int i = isPlayer1 ? 0 : 1;
            Buffer::held[i] = down;

            auto player = isPlayer1 ? m_player1 : m_player2;
            if (down && Buffer::ticks() > 0 && canBuffer(player)) {
                Buffer::pending[i] = player->m_isOnGround ? 0 : Buffer::ticks();
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);

        for (int i = 0; i < 2; ++i) {
            bool p1 = (i == 0);
            auto player = p1 ? m_player1 : m_player2;

            if (Buffer::releaseIn[i] > 0 && --Buffer::releaseIn[i] == 0) {
                GJBaseGameLayer::handleButton(false, 1, p1);
            }

            int& p = Buffer::pending[i];
            if (p <= 0) continue;
            if (!player || !canBuffer(player)) { p = 0; continue; }

            if (player->m_isOnGround) {
                GJBaseGameLayer::handleButton(true, 1, p1);
                if (!Buffer::held[i]) Buffer::releaseIn[i] = 1;
                Buffer::saved++;
                p = 0;
            } else {
                --p;
            }
        }
    }
};

class $modify(BufferPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* label = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        Buffer::clear();
        Buffer::saved = 0;

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.4f);
        label->setOpacity(160);
        label->setAnchorPoint({0.f, 1.f});
        auto win = CCDirector::get()->getWinSize();
        label->setPosition({8.f, win.height - 8.f});
        m_uiLayer->addChild(label, 100);
        m_fields->label = label;
        updateLabel();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        Buffer::clear();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        updateLabel();
    }

    void updateLabel() {
        auto label = m_fields->label;
        if (!label) return;
        bool show = Mod::get()->getSettingValue<bool>("show-overlay");
        label->setVisible(show);
        if (!show) return;

        label->setString(
            Buffer::ticks() == 0
                ? "Buffer: OFF"
                : fmt::format("Buffer: {} ticks | Salvados: {}",
                              Buffer::ticks(), Buffer::saved).c_str()
        );
    }
};

class $modify(BufferKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat) {
        if (down && !repeat && key == KEY_F6) {
            Buffer::levelIdx = (Buffer::levelIdx + 1) % 4;
            Buffer::clear();
            log::info("Input Buffer: {} ticks", Buffer::ticks());
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
    }
};
