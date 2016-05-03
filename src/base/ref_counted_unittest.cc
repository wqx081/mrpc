#include "base/ref_counted.h"
#include "test/opaque_ref_counted.h"
#include <gtest/gtest.h>

using namespace mrpc;

namespace {

class SelfAssign : public RefCounted<SelfAssign> {
 protected:
  virtual ~SelfAssign() {}

 private:
  friend class RefCounted<SelfAssign>;
};

class Derived : public SelfAssign {
 protected:
  ~Derived() override {}

 private:
  friend class RefCounted<Derived>;
};

class CheckDerivedMemberAccess : public scoped_refptr<SelfAssign> {
 public:
  CheckDerivedMemberAccess() {
    SelfAssign** pptr = &ptr_;
    EXPECT_EQ(*pptr, ptr_);
  }
};

class ScopedRefPtrToSelf : public RefCounted<ScopedRefPtrToSelf> { 
 public:
  ScopedRefPtrToSelf() : self_ptr_(this) {}
  static bool was_destroyed() { return was_destroyed_; } 
  static void reset_was_destroyed() { was_destroyed_ = false; } 
  scoped_refptr<ScopedRefPtrToSelf> self_ptr_;
 private:
  friend class RefCounted<ScopedRefPtrToSelf>;
  ~ScopedRefPtrToSelf() { was_destroyed_ = true; } 
  static bool was_destroyed_;
};

bool ScopedRefPtrToSelf::was_destroyed_ = false;

class ScopedRefPtrCountBase : public RefCounted<ScopedRefPtrCountBase> { 
 public:
  ScopedRefPtrCountBase() { ++constructor_count_; } 
  static int constructor_count() { return constructor_count_; } 
  static int destructor_count() { return destructor_count_; } 
  static void reset_count() { 
    constructor_count_ = 0;
    destructor_count_ = 0;
  } 
 protected:
  virtual ~ScopedRefPtrCountBase() { ++destructor_count_; } 
 private:
  friend class RefCounted<ScopedRefPtrCountBase>;
  static int constructor_count_;
  static int destructor_count_;
};

int ScopedRefPtrCountBase::constructor_count_ = 0;
int ScopedRefPtrCountBase::destructor_count_ = 0;


class ScopedRefPtrCountDerived : public ScopedRefPtrCountBase { 
 public:
  ScopedRefPtrCountDerived() { ++constructor_count_; } 
  static int constructor_count() { return constructor_count_; } 
  static int destructor_count() { return destructor_count_; }
  static void reset_count() {
  constructor_count_ = 0;
  destructor_count_ = 0;
}

 protected:
  ~ScopedRefPtrCountDerived() override { ++destructor_count_; }
 private:
  friend class RefCounted<ScopedRefPtrCountDerived>;
  static int constructor_count_;
  static int destructor_count_;
};

int ScopedRefPtrCountDerived::constructor_count_ = 0;
int ScopedRefPtrCountDerived::destructor_count_ = 0;

} // namespace

TEST(RefCountedUnitTest, TestSelfAssignment) {
  SelfAssign* p = new SelfAssign;
  scoped_refptr<SelfAssign> var(p);
  var = var;
  EXPECT_EQ(var.get(), p);
}

TEST(RefCountedUnitTest, ScopedRefPtrMemberAccess) {
  CheckDerivedMemberAccess check;
}

TEST(RefCountedUnitTest, ScopedRefPtrToSelfPointerAssignment) {
  ScopedRefPtrToSelf::reset_was_destroyed();

  ScopedRefPtrToSelf* check = new ScopedRefPtrToSelf();
  EXPECT_FALSE(ScopedRefPtrToSelf::was_destroyed());
  check->self_ptr_ = nullptr;
  EXPECT_TRUE(ScopedRefPtrToSelf::was_destroyed());
}

TEST(RefCountedUnitTest, ScopedRefPtrToSelfMoveAssignment) {
  ScopedRefPtrToSelf::reset_was_destroyed();

  ScopedRefPtrToSelf* check = new ScopedRefPtrToSelf();
  EXPECT_FALSE(ScopedRefPtrToSelf::was_destroyed());
  check->self_ptr_ = scoped_refptr<ScopedRefPtrToSelf>();
  EXPECT_TRUE(ScopedRefPtrToSelf::was_destroyed());
} 

TEST(RefCountedUnitTest, ScopedRefPtrToOpaque) {
  scoped_refptr<OpaqueRefCounted> p = MakeOpaqueRefCounted();
  TestOpaqueRefCounted(p);
  
  scoped_refptr<OpaqueRefCounted> q;
  q = p;
  TestOpaqueRefCounted(p);
  TestOpaqueRefCounted(q);
} 

TEST(RefCountedUnitTest, BooleanTesting) {
  scoped_refptr<SelfAssign> p;
  EXPECT_FALSE(p);
  p = new SelfAssign;
  EXPECT_TRUE(p);
} 

TEST(RefCountedUnitTest, Equality) {
  scoped_refptr<SelfAssign> p1(new SelfAssign);
  scoped_refptr<SelfAssign> p2(new SelfAssign);
  
  EXPECT_EQ(p1, p1);
  EXPECT_EQ(p2, p2);
  
  EXPECT_NE(p1, p2);
  EXPECT_NE(p2, p1);
}

TEST(RefCountedUnitTest, ConvertibleEquality) {
  scoped_refptr<Derived> p1(new Derived);
  scoped_refptr<SelfAssign> p2;

  EXPECT_NE(p1, p2);
  EXPECT_NE(p2, p1);

  p2 = p1;

  EXPECT_EQ(p1, p2);
  EXPECT_EQ(p2, p1);
}

TEST(RefCountedUnitTest, MoveAssignment1) {
  ScopedRefPtrCountBase::reset_count();

  {
    ScopedRefPtrCountBase *raw = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1(raw);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());

    {
      scoped_refptr<ScopedRefPtrCountBase> p2;

      p2 = std::move(p1);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(nullptr, p1.get());
      EXPECT_EQ(raw, p2.get());

    }
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());

  }
  EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
}


TEST(RefCountedUnitTest, MoveAssignment2) {
  ScopedRefPtrCountBase::reset_count();

  {
    ScopedRefPtrCountBase *raw = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1;
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());

    {
      scoped_refptr<ScopedRefPtrCountBase> p2(raw);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());

      p1 = std::move(p2);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(raw, p1.get());
      EXPECT_EQ(nullptr, p2.get());
    }
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
  } 
  EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
} 

TEST(RefCountedUnitTest, MoveAssignmentSameInstance1) {
  ScopedRefPtrCountBase::reset_count();
  
  {
    ScopedRefPtrCountBase *raw = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1(raw);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
    
    {
      scoped_refptr<ScopedRefPtrCountBase> p2(p1);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      
      p1 = std::move(p2);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(raw, p1.get());
      EXPECT_EQ(nullptr, p2.get());
    } 
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
  } 
  EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
} 
TEST(RefCountedUnitTest, MoveAssignmentSameInstance2) {
  ScopedRefPtrCountBase::reset_count();
  
  {
    ScopedRefPtrCountBase *raw = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1(raw);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
    
    {
      scoped_refptr<ScopedRefPtrCountBase> p2(p1);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      
      p2 = std::move(p1);
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(nullptr, p1.get());
      EXPECT_EQ(raw, p2.get());
    } 
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
  } 
  EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
} 

TEST(RefCountedUnitTest, MoveAssignmentDifferentInstances) {
  ScopedRefPtrCountBase::reset_count();
  
  {
    ScopedRefPtrCountBase *raw1 = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1(raw1);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
    
    {
      ScopedRefPtrCountBase *raw2 = new ScopedRefPtrCountBase();
      scoped_refptr<ScopedRefPtrCountBase> p2(raw2);
      EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      
      p1 = std::move(p2);
      EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(raw2, p1.get());
      EXPECT_EQ(nullptr, p2.get());
    } 
    EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
  } 
  EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(2, ScopedRefPtrCountBase::destructor_count());
} 

TEST(RefCountedUnitTest, MoveAssignmentDerived) {
  ScopedRefPtrCountBase::reset_count();
  ScopedRefPtrCountDerived::reset_count();
  
  {
    ScopedRefPtrCountBase *raw1 = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1(raw1);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountDerived::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountDerived::destructor_count());
    
    {
      ScopedRefPtrCountDerived *raw2 = new ScopedRefPtrCountDerived();
      scoped_refptr<ScopedRefPtrCountDerived> p2(raw2);
      EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountDerived::destructor_count());
      
      p1 = std::move(p2);
      EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountDerived::destructor_count());
      EXPECT_EQ(raw2, p1.get());
      EXPECT_EQ(nullptr, p2.get());
    } 
    EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountDerived::destructor_count());
  } 
  EXPECT_EQ(2, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(2, ScopedRefPtrCountBase::destructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountDerived::destructor_count());
} 


TEST(RefCountedUnitTest, MoveConstructor) {
  ScopedRefPtrCountBase::reset_count();
  
  {
    ScopedRefPtrCountBase *raw = new ScopedRefPtrCountBase();
    scoped_refptr<ScopedRefPtrCountBase> p1(raw);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
    
    {
      scoped_refptr<ScopedRefPtrCountBase> p2(std::move(p1));
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(nullptr, p1.get());
      EXPECT_EQ(raw, p2.get());
    } 
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
  } 
  EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
} 

TEST(RefCountedUnitTest, MoveConstructorDerived) {
  ScopedRefPtrCountBase::reset_count();
  ScopedRefPtrCountDerived::reset_count();
  
  {
    ScopedRefPtrCountDerived *raw1 = new ScopedRefPtrCountDerived();
    scoped_refptr<ScopedRefPtrCountDerived> p1(raw1);
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
    EXPECT_EQ(0, ScopedRefPtrCountDerived::destructor_count());
    
    {
      scoped_refptr<ScopedRefPtrCountBase> p2(std::move(p1));
      EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountBase::destructor_count());
      EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
      EXPECT_EQ(0, ScopedRefPtrCountDerived::destructor_count());
      EXPECT_EQ(nullptr, p1.get());
      EXPECT_EQ(raw1, p2.get());
    } 
    EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
    EXPECT_EQ(1, ScopedRefPtrCountDerived::destructor_count());
  } 
  EXPECT_EQ(1, ScopedRefPtrCountBase::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountBase::destructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountDerived::constructor_count());
  EXPECT_EQ(1, ScopedRefPtrCountDerived::destructor_count());
} 
