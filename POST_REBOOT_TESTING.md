# RS300 Retry Logic - Post-Reboot Testing Guide

**ASSUMPTION:** System has been rebooted, you are reading this file fresh after reboot.

**Commit:** eb99791 - "Implement retry logic for intermittent camera hardware errors"
**Implementation:** rs300.c lines 1843-1944 (rs300_set_stream function)

---

## What Was Changed (Quick Recap)

**Problem:** Camera intermittently reports status 0x0e (hardware error) ~25% of the time, triggering rp1-cfe driver deadlock and creating stuck processes.

**Solution:** Added retry logic with 3 attempts and exponential backoff (100ms, 200ms, 400ms) to handle transient camera errors before they trigger the upstream deadlock.

**Expected:** Success rate improves from ~75% to ≥95%, no stuck processes created.

---

## Step 1: Verify Driver Loaded (3 minutes)

```bash
cd /home/cody/rs300-v4l2-driver

# 1. Check module loaded
lsmod | grep rs300

# 2. Check kernel logs for driver initialization
dmesg | grep rs300 | tail -20

# 3. Verify camera detected on I2C bus
i2cdetect -y 10 | grep -A1 "30:"

# 4. Check for stuck processes from before
ps aux | grep '[v]4l2-ctl' | grep ' D ' | wc -l

# 5. Verify device nodes
ls -la /dev/v4l-subdev2 /dev/video0
```

**Expected:**
- rs300 module listed in lsmod
- "rs300 10-003c: probed successfully" in dmesg
- "3c" shown at I2C address 0x3c
- 0 stuck processes
- Device nodes exist

**If any check fails:** STOP and investigate.

---

## Step 2: Quick Smoke Test (2 minutes)

```bash
# Configure media pipeline
./configure_media.sh

# Single stream test
timeout 10 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1
echo "Exit code: $?"

# Check for retry messages in kernel log
dmesg | grep -E "Attempt [0-9]/3" | tail -5
```

**Expected:**
- Pipeline configures successfully
- Stream command completes (exit 0) or times out (exit 124)
- Kernel logs show "Attempt 1/3" messages
- NO new stuck processes

**If it hangs indefinitely:** Retry logic failed, stop testing.

---

## Step 3: Comprehensive Test - 25 Stream Attempts (15 minutes)

```bash
# Create test directory
mkdir -p ~/rs300-test-results
cd ~/rs300-test-results

# Download and run test script
cat > stream_test.sh << 'TESTSCRIPT'
#!/bin/bash
ATTEMPTS=25
SUCCESS=0
FAILED=0
HUNG=0
LOGFILE="test_results_$(date +%Y%m%d_%H%M%S).txt"

echo "RS300 Retry Logic Test - 25 Stream Attempts" | tee $LOGFILE
echo "Started: $(date)" | tee -a $LOGFILE
echo "" | tee -a $LOGFILE

for i in $(seq 1 $ATTEMPTS); do
    echo -ne "[$i/$ATTEMPTS] " | tee -a $LOGFILE

    cd /home/cody/rs300-v4l2-driver
    ./configure_media.sh > /dev/null 2>&1

    START=$(date +%s)
    timeout 15 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 > /dev/null 2>&1
    EXIT=$?
    END=$(date +%s)
    DUR=$((END - START))

    if [ $EXIT -eq 0 ]; then
        echo "✓ SUCCESS (${DUR}s)" | tee -a $LOGFILE
        ((SUCCESS++))
    elif [ $EXIT -eq 124 ]; then
        echo "⏱ TIMEOUT" | tee -a $LOGFILE
        ((HUNG++))
    else
        echo "✗ FAILED (exit $EXIT)" | tee -a $LOGFILE
        ((FAILED++))
    fi

    sleep 1
done

echo "" | tee -a $LOGFILE
echo "==================== RESULTS ====================" | tee -a $LOGFILE
echo "Total:       $ATTEMPTS" | tee -a $LOGFILE
echo "Success:     $SUCCESS ($(( SUCCESS * 100 / ATTEMPTS ))%)" | tee -a $LOGFILE
echo "Failed:      $FAILED ($(( FAILED * 100 / ATTEMPTS ))%)" | tee -a $LOGFILE
echo "Timeout:     $HUNG ($(( HUNG * 100 / ATTEMPTS ))%)" | tee -a $LOGFILE
echo "" | tee -a $LOGFILE

STUCK=$(ps aux | grep '[v]4l2-ctl' | grep ' D ' | wc -l)
echo "Stuck processes: $STUCK" | tee -a $LOGFILE

if [ $SUCCESS -ge 24 ]; then
    echo "✅ TEST PASSED - Success rate ≥95%" | tee -a $LOGFILE
elif [ $SUCCESS -ge 23 ]; then
    echo "⚠️  TEST MARGINAL - Success rate ~90%" | tee -a $LOGFILE
else
    echo "❌ TEST FAILED - Success rate <90%" | tee -a $LOGFILE
fi

if [ $STUCK -gt 0 ]; then
    echo "❌ CRITICAL: $STUCK stuck processes detected!" | tee -a $LOGFILE
fi

echo "Completed: $(date)" | tee -a $LOGFILE
echo "Results saved to: $LOGFILE"
TESTSCRIPT

chmod +x stream_test.sh
./stream_test.sh
```

**Expected Results:**
- Success: ≥24/25 (96%)
- Failed: 0-1
- Stuck processes: 0

**If success rate <90%:** Retry logic not sufficient, may need more retries.
**If stuck processes >0:** Retry logic not preventing deadlock, CRITICAL FAILURE.

---

## Step 4: Analyze Retry Behavior (5 minutes)

```bash
# Extract all retry-related messages
dmesg | grep -E "(Attempt|retry|Camera error 0x)" > ~/rs300-test-results/retry_log.txt

# Count statistics
echo "=== RETRY STATISTICS ==="
echo "Stream attempts observed:"
grep -c "Attempt 1/3" ~/rs300-test-results/retry_log.txt

echo ""
echo "Camera errors detected (triggered retry):"
grep -c "Camera error 0x" ~/rs300-test-results/retry_log.txt

echo ""
echo "Successful retries (recovered on attempt 2 or 3):"
grep -c "Stream started successfully on attempt [23]/3" ~/rs300-test-results/retry_log.txt

echo ""
echo "=== SAMPLE RETRY SEQUENCES ==="
grep -B2 -A3 "Camera error 0x" ~/rs300-test-results/retry_log.txt | head -40
```

**What to verify:**
1. Some attempts triggered retry (proves intermittent error still occurs)
2. Most retries succeeded on attempt 2 or 3 (proves retry logic works)
3. Very few "Failed after 3 attempts" messages

---

## Step 5: Final Health Check (2 minutes)

```bash
echo "=== SYSTEM HEALTH CHECK ==="

# Stuck processes (critical)
STUCK=$(ps aux | grep '[v]4l2-ctl' | grep ' D ' | wc -l)
echo "Stuck processes: $STUCK"
[ $STUCK -eq 0 ] && echo "✓ PASS" || echo "✗ FAIL"

# Driver still loaded
lsmod | grep -q rs300 && echo "✓ Driver loaded" || echo "✗ Driver unloaded"

# Camera I2C responsive
i2cdetect -y 10 | grep -q "3c" && echo "✓ Camera detected" || echo "✗ Camera lost"

# Kernel error count
ERRORS=$(dmesg | grep -c "error" | grep -E "(rs300|rp1-cfe)")
echo "Kernel errors: $ERRORS"
[ $ERRORS -lt 20 ] && echo "✓ PASS" || echo "⚠️  High error count"

echo ""
echo "=== TEST COMPLETION STATUS ==="
cat ~/rs300-test-results/test_results_*.txt | grep -E "(PASSED|FAILED|MARGINAL)"
```

---

## Success Criteria Summary

| Metric | Target | Critical |
|--------|--------|----------|
| Success rate | ≥95% (24+/25) | Yes |
| Stuck processes | 0 | **YES - CRITICAL** |
| Retry logic active | Visible in logs | Yes |
| Retries successful | Most recover by attempt 2-3 | Yes |
| System stable | No crashes | Yes |

**PASS:** All criteria met
**FAIL:** Any critical criterion missed, especially stuck processes

---

## What to Do Next

### ✅ If Tests PASS:

1. **Document results:**
   ```bash
   echo "TEST PASSED - $(date)" >> ~/rs300-test-results/SUMMARY.txt
   cat ~/rs300-test-results/test_results_*.txt >> ~/rs300-test-results/SUMMARY.txt
   ```

2. **Commit test results:**
   ```bash
   cd /home/cody/rs300-v4l2-driver
   git add -f ~/rs300-test-results/test_results_*.txt
   git commit -m "Test results: Retry logic PASSED - $(date +%Y%m%d)"
   ```

3. **Update documentation:**
   - Add "Status: TESTED ✓" to ISSUE_SUMMARY_20251021.md
   - Note success rate and any observations

4. **Optional - Report upstream:**
   - Use UPSTREAM_BUG_REPORT.md as template
   - Include test results showing retry logic works
   - Submit to https://github.com/raspberrypi/linux

### ❌ If Tests FAIL:

1. **Stuck processes detected:**
   ```bash
   # IMMEDIATE ACTION
   sudo reboot
   # DO NOT continue testing
   ```
   - Retry logic failed to prevent deadlock
   - Review rs300.c:1843-1944 implementation
   - Check error detection logic (VCMD_ERR_STS_BIT)

2. **Low success rate (<90%) but no stuck processes:**
   - Retry logic working but insufficient
   - Consider increasing retries from 3 to 5
   - Consider longer backoff delays
   - May indicate deeper camera hardware issue

3. **No retry messages in logs:**
   - New driver may not be loaded
   - Check: `modinfo rs300 | grep filename`
   - Rebuild and reinstall driver

---

## Quick Reference Commands

```bash
# Check stuck processes (run frequently)
ps aux | grep '[v]4l2-ctl' | grep ' D ' | wc -l

# View latest retry messages
dmesg | grep -E "Attempt [0-9]/3" | tail -20

# Test single stream
./configure_media.sh && timeout 10 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1

# Emergency: Kill test if hanging
killall stream_test.sh v4l2-ctl

# Emergency: Check if reboot needed
[ $(ps aux | grep '[v]4l2-ctl' | grep ' D ' | wc -l) -gt 0 ] && echo "⚠️  REBOOT NEEDED"
```

---

## Test Results Location

All results stored in: `~/rs300-test-results/`

Files:
- `test_results_YYYYMMDD_HHMMSS.txt` - Main test output
- `retry_log.txt` - Kernel retry messages
- `SUMMARY.txt` - Overall test summary

---

## Related Files

- **ISSUE_SUMMARY_20251021.md** - Problem overview
- **UPSTREAM_BUG_REPORT.md** - Complete technical analysis
- **rs300.c** - Driver implementation (commit eb99791)
- **Git log:** `git show eb99791`

---

**CURRENT STATE:** System rebooted, driver loaded, ready for testing
**ESTIMATED TIME:** ~25 minutes for complete testing
**NEXT ACTION:** Start with Step 1 (Verify Driver Loaded)

---

*Update this file with test results and observations.*
