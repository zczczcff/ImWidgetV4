#include <imwidgetv4/reflection/ReflectionTypes.h>

#include <algorithm>

namespace ImWidgetV4::Reflection {

FPropertyHandle::FPropertyHandle(void* owner, const FPropertyDesc* desc)
    : Owner_(owner)
    , Desc_(desc)
{
}

bool FPropertyHandle::IsValid() const
{
    return Owner_ != nullptr && Desc_ != nullptr;
}

const FPropertyDesc* FPropertyHandle::GetDesc() const
{
    return Desc_;
}

const char* FPropertyHandle::GetName() const
{
    return Desc_ ? Desc_->Name : "";
}

const char* FPropertyHandle::GetOwnerTypeName() const
{
    return Desc_ ? Desc_->OwnerTypeName : "";
}

EPropertyKind FPropertyHandle::GetKind() const
{
    return Desc_ ? Desc_->Kind : EPropertyKind::Struct;
}

void* FPropertyHandle::GetMutablePtr() const
{
    if (!IsValid() || !Desc_->GetMutablePtr) {
        return nullptr;
    }

    return Desc_->GetMutablePtr(Owner_);
}

const void* FPropertyHandle::GetConstPtr() const
{
    if (!IsValid() || !Desc_->GetConstPtr) {
        return nullptr;
    }

    return Desc_->GetConstPtr(Owner_);
}

bool FPropertyHandle::CopyFrom(const void* source) const
{
    if (!IsValid() || !Desc_->CopyValue) {
        return false;
    }

    return Desc_->CopyValue(GetMutablePtr(), source);
}

std::vector<const FPropertyDesc*> CollectProperties(const FTypeDesc& typeDesc)
{
    std::vector<const FPropertyDesc*> result;

    for (size_t index = 0; index < typeDesc.PropertyCount; ++index) {
        result.push_back(&typeDesc.Properties[index]);
    }

    for (const FTypeDesc* parent = typeDesc.Parent; parent; parent = parent->Parent) {
        for (size_t index = 0; index < parent->PropertyCount; ++index) {
            result.push_back(&parent->Properties[index]);
        }
    }

    return result;
}

const FPropertyDesc* FindProperty(
    const FTypeDesc& typeDesc,
    const std::string& name,
    const std::string& ownerTypeName)
{
    const auto properties = CollectProperties(typeDesc);
    const auto it = std::find_if(
        properties.begin(),
        properties.end(),
        [&name, &ownerTypeName](const FPropertyDesc* property) {
            if (!property || name != property->Name) {
                return false;
            }

            return ownerTypeName.empty() || ownerTypeName == property->OwnerTypeName;
        });

    return it != properties.end() ? *it : nullptr;
}

} // namespace ImWidgetV4::Reflection
