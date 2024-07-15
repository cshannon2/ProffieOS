#ifndef MODE_SETTINGS_MENUES_H
#define MODE_SETTINGS_MENUES_H

#include "../common/color.h"
#include "menu_list.h"
#include "bool_setting.h"

int prop_GetCurrentClashThreshold();
void prop_SetClashThreshold(int clash_threshold);
int prop_GetBladeLength(int blade);
int prop_GetMaxBladeLength(int blade);
void prop_SetBladeLength(int blade, int len);

namespace mode {

#ifndef VOLUME_MENU_GAMMA
#define VOLUME_MENU_GAMMA 2.2
#endif

#define SETTINGS_MENU_HAS_ONLY_ONE_ENTRY
  
template<class SPEC>
struct SmoothVolumeMode : public SPEC::SmoothMode {
  virtual float revolutions() { return 1.0f; }
  // x = 0-32767
  virtual int get() override {
    float ret = dynamic_mixer.get_volume() / VOLUME;
    ret = powf(ret, 1.0 / VOLUME_MENU_GAMMA);
    return ret * 32767.0;
  }
    
  virtual void set(int x) {
    float ret = x / 32768.0f;
    if (!getSL<SPEC>()->busy()) {
      getSL<SPEC>()->SayWhole(ret * 100);
      getSL<SPEC>()->SayPercent();

      PVLOG_VERBOSE << " x = " << x
		    << " angle_ = " << (float)this->angle_
		    << " ret = " << ret
		    << " bent = " << powf(ret, VOLUME_MENU_GAMMA)
		    << " final= " << VOLUME * powf(ret, VOLUME_MENU_GAMMA)
		    << "\n";
    }

    ret = powf(ret, VOLUME_MENU_GAMMA);
    dynamic_mixer.set_volume(VOLUME * ret);
  }

  void exit() override {
    getSL<SPEC>()->SayCancel();
    SPEC::SmoothMode::exit();
  }
};

#ifdef DYNAMIC_BLADE_LENGTH
#undef SETTINGS_MENU_HAS_ONLY_ONE_ENTRY  

template<class SPEC>
class ShowLengthStyle {
  void run(BladeBase* blade) {
    len_ = getPtr<SPEC::ChangeBladeLength>()->getLength();
  }
  OverDriveColor getColor(int led) {
    if (led == len_ - 1) return SimpleColor(Color16(65535,65535,65535));
    return SimpleColor(Color16(0,0,0));
  }
  int len_;
};

template<class SPEC>
struct ChangeBladeLengthBlade1 : public SPEC::MenuBase {
  // TODO: get the blade from the spec??
  virtual int blade() { return 1; }
  void activate(bool onreturn) {
    SPEC::MenuBase::activate(onreturn);
    int len = prop_GetBladeLength(blade());
    int maxlen = prop_GetMaxBladeLength(blade());
    saved_len_ = len;
    if (len == -1) len = maxlen;
    this->pos_ = len;
    SetBladeLength(blade(), maxlen);
    showlen_.Start(blade());
  }
  void say() override {
    getSL<SPEC>()->SayWhole(this->pos_);
  }
  uint16_t size() override {
    return prop_GetMaxBladeLength(blade());
  }
  void select() override {
    // SPEC::MenuBase::select(); = 0
    prop_SetBladeLength(blade(), this->pos_);
    getSL<SPEC>()->SaySelect();
    showlen_.Stop(blade());
    prop_SaveState();
  }
  void exit() override {
    getSL<SPEC>()->SayCancel();
    SPEC::MenuBase::exit();
    prop_SetBladeLength(blade(), saved_len_);
    showlen_.Stop(blade());
  }

  int getLength() { return this->pos_; }

  ShowColorSingleBladeTemplate<typename SPEC::ShowLengthStyle> showlen_;
  int saved_len_;
};
#endif

#ifdef DYNAMIC_CLASH_THRESHOLD
#undef SETTINGS_MENU_HAS_ONLY_ONE_ENTRY  

#ifndef CLASH_THRESHOLD_GAMMA
#define CLASH_THRESHOLD_GAMMA 2.0f
#endif

#define MAX_CLASH_THRESHOLD_G 16.0f

template<class SPEC>
struct SmoothClashThresholdMode : public SPEC::SmoothMode {
  void activate(bool onreturn) {
    SPEC::SmoothMode::activate(onreturn);
  }
  // x = 0-32767
  virtual int get() override {
    float ret = prop_GetCurrentClashThreshold() / MAX_CLASH_THRESHOLD_G;
    ret = powf(ret, 1.0/CLASH_THRESHOLD_GAMMA);
    return ret * 32768.0;
  }
    
  virtual void set(int x) {
    value_ = x;
    if (!getSL<SPEC>()->busy()) {
      float ret = value_ / 32768.0f;
      ret = powf(ret, CLASH_THRESHOLD_GAMMA);
      ret *= MAX_CLASH_THRESHOLD_G;
      getSL<SPEC>()->SayNumber(ret, SayType::SAY_DECIMAL);
    }
  }
  void exit() override {
    getSL<SPEC>()->SayCancel();
    SPEC::SmoothMode::exit();
  }
  void select() override {
    float ret = value_ / 32768.0f;
    ret = powf(ret, CLASH_THRESHOLD_GAMMA);
    ret *= MAX_CLASH_THRESHOLD_G;
    prop_SetClashThreshold(ret);
    getSL<SPEC>()->SaySelect();
    SPEC::SmoothMode::select();
  }

  int value_;
};
#endif


#ifdef DYNAMIC_BLADE_DIMMING
#undef SETTINGS_MENU_HAS_ONLY_ONE_ENTRY  

template<class SPEC>
struct SmoothChangeBladeDimmingMode : public SPEC::SmoothMode {
  virtual float revolutions() { return 1.0f; }
  void mode_activate(bool onreturn) {
    SPEC::SmoothMode::mode_activate(onreturn);
    SaberBase::TurnOn();
  }
  void mode_deactivate() override {
    SaberBase::TurnOff(SaberBase::OFF_FAST);
    SPEC::SmoothMode::mode_deactivate();
  }
  // x = 0-32767
  virtual int get() override {
    float ret =  SaberBase::GetCurrentDimming() / 16384.0;
    ret = powf(ret, 1.0/COLOR_MENU_GAMMA);
    return ret * 32767.0;
  }
    
  virtual void set(int x) {
    float ret = x / 32767.f;
    ret = powf(ret, COLOR_MENU_GAMMA);
    SaberBase::SetDimming(ret * 16384.0f);
    if (!getSL<SPEC>()->busy()) {
      getSL<SPEC>()->SayWhole(ret * 100);
      getSL<SPEC>()->SayPercent();
    }
  }

  void exit() override {
    getSL<SPEC>()->SayCancel();
    SPEC::SmoothMode::exit();
  }
  void select() override {
    getSL<SPEC>()->SaySelect();
    SPEC::SmoothMode::select();
  }
};

#endif

#ifdef MOUNT_SD_SETTING
#undef SETTINGS_MENU_HAS_ONLY_ONE_ENTRY  

template<class SPEC>
struct AllowMountBoolSetting : public BoolSetting {
  bool get() override { return LSFS::GetAllowMount(); }
  void set(bool value) override { return LSFS::SetAllowMount(value); }
  void say() override { getSL<SPEC>()->SaySDAccess(); }
};
#endif

template<class SPEC>
struct SettingsMenuMode : public MenuEntryMenu<SPEC,
  SubMenuEntry<typename SPEC::ChangeVolumeMode, typename SPEC::SoundLibrary::tEditVolume>
#ifdef MOUNT_SD_SETTING
  // TODO: make this more configurable
  ,DirectBoolEntry<SPEC, AllowMountBoolSetting<SPEC>>
#endif
#ifdef DYNAMIC_BLADE_LENGTH
  ,SubMenuEntry<typename SPEC::ChangeBladeLengthMode, typename SPEC::SoundLibrary::tEditBladeLength>
#endif
#ifdef DYNAMIC_CLASH_THRESHOLD
  ,SubMenuEntry<typename SPEC::ChangeClashThresholdMode, typename SPEC::SoundLibrary::tEditClashThreshold>
#endif  
#ifdef DYNAMIC_BLADE_DIMMING
  ,SubMenuEntry<typename SPEC::ChangeBladeDimmingMode, typename SPEC::SoundLibrary::tEditBrightness>
#endif

> {};

template<class SPEC>
class BasicTopMenu : public MenuEntryMenu<SPEC,
  SubMenuEntry<typename SPEC::EditPresetsMenu, typename SPEC::SoundLibrary::tEditPresets>,
#ifdef SETTINGS_MENU_HAS_ONLY_ONE_ENTRY  
  SubMenuEntry<typename SPEC::ChangeVolumeMode, typename SPEC::SoundLibrary::tEditVolume>
#else
  SubMenuEntry<typename SPEC::SettingsMenu, typename SPEC::SoundLibrary::tEditSettings>
#endif
> {};

}  // namespace mode

#endif
