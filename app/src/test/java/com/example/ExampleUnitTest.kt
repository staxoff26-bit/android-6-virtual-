package com.example

import org.junit.Assert.assertEquals
import org.junit.Test

class ExampleUnitTest {
  @Test
  fun addition_isCorrect() {
    assertEquals(4, 2 + 2)
  }

  @Test
  fun testFormatBytes() {
    assertEquals("0 B", DeviceAudit.formatBytes(0))
    assertEquals("1023 B", DeviceAudit.formatBytes(1023))
    assertEquals("1.00 KB", DeviceAudit.formatBytes(1024))
    assertEquals("1.00 MB", DeviceAudit.formatBytes(1024 * 1024))
    assertEquals("512.00 MB", DeviceAudit.formatBytes(512L * 1024 * 1024))
    assertEquals("2.00 GB", DeviceAudit.formatBytes(2L * 1024 * 1024 * 1024))
  }
}
