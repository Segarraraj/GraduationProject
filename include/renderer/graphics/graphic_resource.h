#ifndef __GRAPHIC_RESOURCE_H__
#define __GRAPHIC_RESOURCE_H__ 1

namespace RR {
class GraphicResource {
 public:
  GraphicResource() = default;
  virtual ~GraphicResource() = default;

  bool Updated() const;
  bool Initialized() const;

 protected:
  bool _updated = false;
  bool _initialized = false;

  virtual void Release() = 0;
};
}

#endif  // !__GRAPHIC_RESOURCE_H__
