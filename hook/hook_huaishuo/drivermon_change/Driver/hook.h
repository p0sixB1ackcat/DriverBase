void StartHook (void);
void RemoveHook (void);

NTSTATUS
NTAPI HOOK_NtCreateSection(
    OUT PHANDLE             SectionHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER       MaximumSize OPTIONAL,
    IN ULONG                SectionPageProtection,
    IN ULONG                AllocationAttributes,
    IN HANDLE               FileHandle OPTIONAL
);