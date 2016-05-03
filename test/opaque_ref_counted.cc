#include "test/opaque_ref_counted.h"
#include "base/macros.h"
#include <gtest/gtest.h>

namespace mrpc {

class OpaqueRefCounted : public RefCounted<OpaqueRefCounted> {
 public:
  OpaqueRefCounted() {}

  int Return42() { return 42; }

 private:
  virtual ~OpaqueRefCounted() {}

  friend RefCounted<OpaqueRefCounted>;
  DISALLOW_COPY_AND_ASSIGN(OpaqueRefCounted);
};

scoped_refptr<OpaqueRefCounted> MakeOpaqueRefCounted() {
  return new OpaqueRefCounted();
}

void TestOpaqueRefCounted(scoped_refptr<OpaqueRefCounted> p) {
  EXPECT_EQ(42, p->Return42());
}

}  // namespace base

template class scoped_refptr<mrpc::OpaqueRefCounted>;
