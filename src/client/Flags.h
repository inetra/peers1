#ifndef DCPLUSPLUS_CLIENT_FLAGS_H_
#define DCPLUSPLUS_CLIENT_FLAGS_H_

class Flags {
	public:
		typedef uint16_t MaskType;

		Flags() : flags(0) { }
		Flags(const Flags& rhs) : flags(rhs.flags) { }
		Flags(int f) : flags(MaskType(f)) { } //[+]PPA
		Flags(MaskType f) : flags(f) { }

#ifdef __INTEL_COMPILER
		virtual ~Flags() throw() {}
#endif
		bool isSet(int aFlag) const { return (flags & MaskType(aFlag)) == aFlag; } //[+]PPA
		bool isSet(MaskType aFlag) const { return (flags & aFlag) == aFlag; }
		bool isAnySet(MaskType aFlag) const { return (flags & aFlag) != 0; }
		bool isAnySet(int aFlag) const { return (flags & MaskType(aFlag)) != 0; } //[+]PPA
		void setFlag(MaskType aFlag) { flags |= aFlag; }
		void setFlag(int aFlag) { flags |= MaskType(aFlag); } //[+]PPA
		void unsetFlag(MaskType aFlag) { flags &= ~aFlag; }
		void unsetFlag(int aFlag) { flags &= ~MaskType(aFlag); } //[+]PPA
        void setFlags(MaskType aFlags) { flags = aFlags; } // !SMT!
		MaskType getFlags() const { return flags; }
		Flags& operator=(const Flags& rhs) { flags = rhs.flags; return *this; }
	private:
		MaskType flags;
};


#endif /*FLAGS_H_*/
