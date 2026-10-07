/** L2CAP security levels, `BT_SECURITY_*` in the kernel */
export const security: {
  readonly LOW: number
  readonly MEDIUM: number
  readonly HIGH: number
  readonly FIPS: number
}

/** Whether `level` is one of `security` */
export function isSecurity(level: number): boolean
