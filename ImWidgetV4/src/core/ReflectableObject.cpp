#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/reflection/Reflectable.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>

namespace ImWidgetV4 {

FReflectedOptionalProperty::FReflectedOptionalProperty(ReflectableObject* owner, const Reflection::FPropertyDesc* desc)
    : Owner_(owner)
    , Desc_(desc)
{
    if (!desc || desc->Kind != Reflection::EPropertyKind::Enum) {
        return;
    }

    Options_.reserve(desc->EnumOptions.Count);
    for (size_t index = 0; index < desc->EnumOptions.Count; ++index) {
        Options_.emplace_back(desc->EnumOptions.Names[index]);
    }
}

bool FReflectedOptionalProperty::IsValid() const
{
    return Owner_ && Desc_ && Desc_->Kind == Reflection::EPropertyKind::Enum;
}

std::string FReflectedOptionalProperty::GetOptionString() const
{
    if (!Owner_ || !Desc_) {
        return std::string();
    }

    Reflection::FPropertyHandle handle(Owner_, Desc_);
    Reflection::FPropertyValue value;
    if (!handle.Read(value) || value.IntValue < 0 || static_cast<size_t>(value.IntValue) >= Options_.size()) {
        return std::string();
    }

    return Options_[value.IntValue];
}

const std::vector<std::string>& FReflectedOptionalProperty::GetOptionList() const
{
    return Options_;
}

bool FReflectedOptionalProperty::SetOptionByString(const std::string& option)
{
    for (size_t index = 0; index < Options_.size(); ++index) {
        if (Options_[index] == option) {
            return SetOptionByIndex(static_cast<int>(index));
        }
    }

    return false;
}

bool FReflectedOptionalProperty::SetOptionByIndex(int index)
{
    if (!Owner_ || !Desc_ || index < 0 || static_cast<size_t>(index) >= Options_.size()) {
        return false;
    }

    Reflection::FPropertyHandle handle(Owner_, Desc_);
    Reflection::FPropertyValue value;
    value.Kind = Reflection::EPropertyKind::Enum;
    value.IntValue = index;
    return handle.Write(value);
}

size_t FReflectedOptionalProperty::GetOptionCount() const
{
    return Options_.size();
}

const Reflection::FTypeDesc& ReflectableObject::GetTypeDesc() const
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        return *typeDesc;
    }

    static const Reflection::FTypeDesc fallbackTypeDesc {
        "ReflectableObject",
        nullptr,
        nullptr,
        0
    };
    return fallbackTypeDesc;
}

const Reflection::FTypeDesc* ReflectableObject::FindReflectionTypeDesc() const
{
    return Reflection::FReflectionRegistry::Get().FindType(GetTypeName());
}

bool ReflectableObject::HasProperty(const std::string& name) const
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (Reflection::FindProperty(*typeDesc, name)) {
            return true;
        }
    }

    return false;
}

bool ReflectableObject::HasProperty(const std::string& name, const std::string& className) const
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (Reflection::FindProperty(*typeDesc, name, className)) {
            return true;
        }
    }

    return false;
}

FReflectedOptionalProperty ReflectableObject::GetPropertyAsOptional(const std::string& name)
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (const Reflection::FPropertyDesc* property = Reflection::FindProperty(*typeDesc, name)) {
            return FReflectedOptionalProperty(this, property);
        }
    }

    return FReflectedOptionalProperty();
}

FReflectedOptionalProperty ReflectableObject::GetPropertyAsOptional(const std::string& name, const std::string& className)
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (const Reflection::FPropertyDesc* property = Reflection::FindProperty(*typeDesc, name, className)) {
            return FReflectedOptionalProperty(this, property);
        }
    }

    return FReflectedOptionalProperty();
}

} // namespace ImWidgetV4
